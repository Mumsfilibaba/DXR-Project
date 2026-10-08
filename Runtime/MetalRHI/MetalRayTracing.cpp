#include "MetalRHI/MetalRayTracing.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQueue.h"
#include "RHI/RHIStats.h"
#include "Core/Math/Math.h"
#include "Core/Threading/ScopedLock.h"

static MTLAccelerationStructureUsage ConvertAccelerationStructureUsage(EAccelerationStructureBuildFlags Flags)
{
    MTLAccelerationStructureUsage Usage = MTLAccelerationStructureUsageNone;
    if (IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate))
    {
        Usage |= MTLAccelerationStructureUsageRefit;
    }

    if (IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild))
    {
        Usage |= MTLAccelerationStructureUsagePreferFastBuild;
    }

    return Usage;
}

static MTLAccelerationStructureInstanceOptions ConvertInstanceFlags(ERayTracingInstanceFlags Flags)
{
    MTLAccelerationStructureInstanceOptions Options = MTLAccelerationStructureInstanceOptionNone;
    if (IsEnumFlagSet(Flags, ERayTracingInstanceFlags::CullDisable))
    {
        Options |= MTLAccelerationStructureInstanceOptionDisableTriangleCulling;
    }

    if (IsEnumFlagSet(Flags, ERayTracingInstanceFlags::FrontCounterClockwise))
    {
        Options |= MTLAccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise;
    }

    if (IsEnumFlagSet(Flags, ERayTracingInstanceFlags::ForceOpaque))
    {
        Options |= MTLAccelerationStructureInstanceOptionOpaque;
    }

    if (IsEnumFlagSet(Flags, ERayTracingInstanceFlags::ForceNonOpaque))
    {
        Options |= MTLAccelerationStructureInstanceOptionNonOpaque;
    }

    return Options;
}

static MTLPackedFloat4x3 ConvertTransform(const Matrix3x4& Transform)
{
    MTLPackedFloat4x3 Result;
    for (int32 Column = 0; Column < 4; ++Column)
    {
        for (int32 Row = 0; Row < 3; ++Row)
        {
            Result.columns[Column].elements[Row] = Transform.M[Row][Column];
        }
    }

    return Result;
}

FMetalAccelerationStructure::FMetalAccelerationStructure(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , Storage()
    , PinnedEntry(nullptr)
    , DebugName()
    , TrackedMemory(0)
{
}

FMetalAccelerationStructure::~FMetalAccelerationStructure()
{
    GetDevice()->GetResidencyManager().Unpin(PinnedEntry);
    PinnedEntry = nullptr;

    STAT_SUBTRACT(STAT_RHI_AccelerationStructureMemory, TrackedMemory);
}

void FMetalAccelerationStructure::UpdateMemoryStat()
{
    const int64 NewSize = Storage ? static_cast<int64>(Storage->GetSize()) : 0;
    STAT_ADD(STAT_RHI_AccelerationStructureMemory, NewSize - TrackedMemory);
    TrackedMemory = NewSize;
}

bool FMetalAccelerationStructure::CompactInPlace(FMetalCommandContext& Context, uint64 CompactedSizeInBytes)
{
    id<MTLAccelerationStructure> Source = GetMTLAccelerationStructure();
    if (!Source || CompactedSizeInBytes == 0)
    {
        return false;
    }

    const MTLSizeAndAlign SizeAndAlign = [GetDevice()->GetMTLDevice() heapAccelerationStructureSizeAndAlignWithSize:CompactedSizeInBytes];
    return EncodeCopy(Context, Source, GetResidencyEntry(), SizeAndAlign, true);
}

bool FMetalAccelerationStructure::CopyFrom(FMetalCommandContext& Context, const FMetalAccelerationStructure& Source, bool bCompact)
{
    CHECK(&Source != this);

    id<MTLAccelerationStructure> SourceStructure = Source.GetMTLAccelerationStructure();
    if (!SourceStructure)
    {
        return false;
    }

    const MTLSizeAndAlign SizeAndAlign = [GetDevice()->GetMTLDevice() heapAccelerationStructureSizeAndAlignWithSize:SourceStructure.size];
    return EncodeCopy(Context, SourceStructure, Source.GetResidencyEntry(), SizeAndAlign, bCompact);
}

bool FMetalAccelerationStructure::ReserveStorage(FMetalCommandContext& Context, MTLSizeAndAlign SizeAndAlign)
{
    if (Storage && Storage->GetSize() >= SizeAndAlign.size)
    {
        return true;
    }

    TUniquePtr<FMetalResourceStorage> NewStorage = AllocateStorage(Context, SizeAndAlign);
    if (!NewStorage)
    {
        return false;
    }

    SetStorage(Context, Move(NewStorage));
    return true;
}

TUniquePtr<FMetalResourceStorage> FMetalAccelerationStructure::AllocateStorage(FMetalCommandContext& Context, MTLSizeAndAlign SizeAndAlign)
{
    TUniquePtr<FMetalResourceStorage> NewStorage = MakeUniquePtr<FMetalResourceStorage>(GetDevice());
    if (!GetDevice()->GetBufferAllocator()->TryAllocateAccelerationStructure(SizeAndAlign, *NewStorage))
    {
        return nullptr;
    }

    if (NewStorage->GetStorageType() == EMetalResourceStorageType::SuballocatedHeap)
    {
        id<MTLBuffer> Alias = [NewStorage->GetHeap()->GetMTLHeap() newBufferWithLength:NewStorage->GetSize()
                                                                               options:MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked
                                                                                offset:NewStorage->GetResourceOffset()];
        if (!Alias)
        {
            METAL_ERROR("Failed to alias %llu bytes of acceleration structure storage for clearing", NewStorage->GetSize());
            return nullptr;
        }

        FMetalEncoderManager& Encoders = Context.GetEncoders();
        Encoders.UpdateResidency(NewStorage->GetResidencyEntry());
        [Encoders.RequireBlitEncoder() fillBuffer:Alias range:NSMakeRange(0, NewStorage->GetSize()) value:0];

        Encoders.GetCommands().DeferredObjects.Emplace(static_cast<id<NSObject>>(Alias));
        [Alias release];
    }

    return NewStorage;
}

bool FMetalAccelerationStructure::EncodeBuild(FMetalCommandContext& Context, MTLAccelerationStructureDescriptor* Descriptor, bool bRefit)
{
    CHECK(Storage != nullptr);

    const MTLAccelerationStructureSizes Sizes       = [GetDevice()->GetMTLDevice() accelerationStructureSizesWithDescriptor:Descriptor];
    const uint64                        ScratchSize = Math::Max<uint64>(bRefit ? Sizes.refitScratchBufferSize : Sizes.buildScratchBufferSize, 1);

    TUniquePtr<FMetalResourceStorage> Scratch = MakeUniquePtr<FMetalResourceStorage>(GetDevice());
    if (!GetDevice()->GetBufferAllocator()->TryAllocate(ScratchSize, 0, MTLResourceStorageModePrivate, false, *Scratch))
    {
        METAL_ERROR("Failed to allocate %llu bytes of acceleration structure scratch", ScratchSize);
        return false;
    }

    FMetalEncoderManager& Encoders = Context.GetEncoders();
    Encoders.UpdateResidency(Storage->GetResidencyEntry());
    Encoders.UpdateResidency(Scratch->GetResidencyEntry());

    id<MTLAccelerationStructureCommandEncoder> Encoder = Encoders.RequireAccelerationStructureEncoder();
    Encoders.RefreshBindlessResidency(Encoder);

    id<MTLAccelerationStructure> AccelerationStructure = Storage->GetAccelerationStructure();
    if (bRefit)
    {
        [Encoder refitAccelerationStructure:AccelerationStructure descriptor:Descriptor destination:AccelerationStructure scratchBuffer:Scratch->GetBuffer() scratchBufferOffset:0];
    }
    else
    {
        [Encoder buildAccelerationStructure:AccelerationStructure descriptor:Descriptor scratchBuffer:Scratch->GetBuffer() scratchBufferOffset:0];
    }

    Encoders.GetCommands().RetireStorage(Move(Scratch), nullptr);
    STAT_ADD(STAT_RHI_AccelerationStructureBuilds, 1);
    return true;
}

void FMetalAccelerationStructure::SetLabel(const String& InName)
{
    DebugName = InName;

    @autoreleasepool
    {
        if (id<MTLAccelerationStructure> AccelerationStructure = GetMTLAccelerationStructure())
        {
            AccelerationStructure.label = DebugName.GetNSString();
        }
    }
}

void FMetalAccelerationStructure::SetStorage(FMetalCommandContext& Context, TUniquePtr<FMetalResourceStorage> NewStorage)
{
    FMetalResidencyEntry* NewEntry = NewStorage ? NewStorage->GetResidencyEntry() : nullptr;
    GetDevice()->GetResidencyManager().Pin(NewEntry);

    Context.GetEncoders().GetCommands().RetireStorage(Move(Storage), PinnedEntry);
    Storage     = Move(NewStorage);
    PinnedEntry = NewEntry;
    UpdateMemoryStat();

    if (!DebugName.IsEmpty())
    {
        SetLabel(DebugName);
    }

    OnStorageReplaced();
}

bool FMetalAccelerationStructure::EncodeCopy(FMetalCommandContext& Context, id<MTLAccelerationStructure> Source, FMetalResidencyEntry* SourceEntry, MTLSizeAndAlign SizeAndAlign, bool bCompact)
{
    TUniquePtr<FMetalResourceStorage> NewStorage = AllocateStorage(Context, SizeAndAlign);
    if (!NewStorage)
    {
        return false;
    }

    FMetalEncoderManager& Encoders = Context.GetEncoders();
    Encoders.UpdateResidency(SourceEntry);
    Encoders.UpdateResidency(NewStorage->GetResidencyEntry());

    id<MTLAccelerationStructureCommandEncoder> Encoder = Encoders.RequireAccelerationStructureEncoder();
    if (bCompact)
    {
        [Encoder copyAndCompactAccelerationStructure:Source toAccelerationStructure:NewStorage->GetAccelerationStructure()];
    }
    else
    {
        [Encoder copyAccelerationStructure:Source toAccelerationStructure:NewStorage->GetAccelerationStructure()];
    }

    SetStorage(Context, Move(NewStorage));
    return true;
}

FMetalGeometryAccelerationStructureRHI::FMetalGeometryAccelerationStructureRHI(FMetalDevice* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
    : FRHIGeometryAccelerationStructure(InGeometryDesc)
    , FMetalAccelerationStructure(InDevice)
{
    STAT_ADD(STAT_RHI_BLASCount, 1);
}

FMetalGeometryAccelerationStructureRHI::~FMetalGeometryAccelerationStructureRHI()
{
    STAT_SUBTRACT(STAT_RHI_BLASCount, 1);
}

void* FMetalGeometryAccelerationStructureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(GetMTLAccelerationStructure());
}

void FMetalGeometryAccelerationStructureRHI::SetDebugName(const String& InName)
{
    SetLabel(InName);
}

void FMetalGeometryAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = GetLabel();
}

bool FMetalGeometryAccelerationStructureRHI::Build(FMetalCommandContext& Context, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    @autoreleasepool
    {
        MTLPrimitiveAccelerationStructureDescriptor* Descriptor = CreateDescriptor(BuildDesc);
        if (!Descriptor)
        {
            return false;
        }

        const bool bRefit = BuildDesc.bUpdate && IsEnumFlagSet(GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate) && GetMTLAccelerationStructure() != nil;
        if (!bRefit)
        {
            const MTLSizeAndAlign SizeAndAlign = [GetDevice()->GetMTLDevice() heapAccelerationStructureSizeAndAlignWithDescriptor:Descriptor];
            if (!ReserveStorage(Context, SizeAndAlign))
            {
                return false;
            }
        }

        FMetalEncoderManager& Encoders = Context.GetEncoders();
        for (FRHIBuffer* Buffer : { BuildDesc.VertexBuffer, BuildDesc.IndexBuffer, BuildDesc.AABBBuffer })
        {
            if (FMetalBufferRHI* MetalBuffer = GetMetalBuffer(Buffer))
            {
                Encoders.UpdateResidency(MetalBuffer->GetResidencyEntry());
            }
        }

        return EncodeBuild(Context, Descriptor, bRefit);
    }
}

MTLPrimitiveAccelerationStructureDescriptor* FMetalGeometryAccelerationStructureRHI::CreateDescriptor(const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc) const
{
    MTLAccelerationStructureGeometryDescriptor* GeometryDescriptor = nil;
    if (GetGeometryType() == ERayTracingGeometryType::ProceduralAABBs)
    {
        FMetalBufferRHI* AABBBuffer = GetMetalBuffer(BuildDesc.AABBBuffer);
        if (!AABBBuffer)
        {
            METAL_ERROR("Procedural geometry requires an AABB buffer");
            return nil;
        }

        MTLAccelerationStructureBoundingBoxGeometryDescriptor* BoundingBoxes = [MTLAccelerationStructureBoundingBoxGeometryDescriptor descriptor];
        BoundingBoxes.boundingBoxBuffer       = AABBBuffer->GetMTLBuffer();
        BoundingBoxes.boundingBoxBufferOffset = AABBBuffer->GetMetalBindOffset();
        BoundingBoxes.boundingBoxStride       = BuildDesc.AABBStride;
        BoundingBoxes.boundingBoxCount        = BuildDesc.NumAABBs;
        GeometryDescriptor = BoundingBoxes;
    }
    else
    {
        FMetalBufferRHI* VertexBuffer = GetMetalBuffer(BuildDesc.VertexBuffer);
        if (!VertexBuffer)
        {
            METAL_ERROR("Triangle geometry requires a vertex buffer");
            return nil;
        }

        MTLAccelerationStructureTriangleGeometryDescriptor* Triangles = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
        Triangles.vertexBuffer       = VertexBuffer->GetMTLBuffer();
        Triangles.vertexBufferOffset = VertexBuffer->GetMetalBindOffset();
        Triangles.vertexStride       = VertexBuffer->GetDesc().Stride;
        Triangles.vertexFormat       = MTLAttributeFormatFloat3;

        FMetalBufferRHI* IndexBuffer = GetMetalBuffer(BuildDesc.IndexBuffer);
        if (IndexBuffer && BuildDesc.NumIndices > 0)
        {
            Triangles.indexBuffer       = IndexBuffer->GetMTLBuffer();
            Triangles.indexBufferOffset = IndexBuffer->GetMetalBindOffset();
            Triangles.indexType         = BuildDesc.IndexFormat == EIndexFormat::uint16 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
            Triangles.triangleCount     = BuildDesc.NumIndices / 3;
        }
        else
        {
            Triangles.triangleCount = BuildDesc.NumVertices / 3;
        }

        GeometryDescriptor = Triangles;
    }

    GeometryDescriptor.opaque = YES;

    MTLPrimitiveAccelerationStructureDescriptor* Descriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
    Descriptor.geometryDescriptors = @[ GeometryDescriptor ];
    Descriptor.usage               = ConvertAccelerationStructureUsage(GetFlags());
    return Descriptor;
}

FMetalSceneAccelerationStructureRHI::FMetalSceneAccelerationStructureRHI(FMetalDevice* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc)
    : FRHISceneAccelerationStructure(InSceneDesc)
    , FMetalAccelerationStructure(InDevice)
    , View(new FMetalShaderResourceViewRHI(InDevice, this, FRHIShaderResourceViewDesc::CreateAccelerationStructure()))
    , BindlessCS()
    , NumBuiltInstances(0)
{
    View->Initialize();
    STAT_ADD(STAT_RHI_TLASCount, 1);
}

FMetalSceneAccelerationStructureRHI::~FMetalSceneAccelerationStructureRHI()
{
    {
        TScopedLock Lock(BindlessCS);
        View->OnResourceReleased();
    }

    STAT_SUBTRACT(STAT_RHI_TLASCount, 1);
}

void* FMetalSceneAccelerationStructureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(GetMTLAccelerationStructure());
}

FRHIShaderResourceView* FMetalSceneAccelerationStructureRHI::GetShaderResourceView() const
{
    return View.Get();
}

FRHIDescriptorHandle FMetalSceneAccelerationStructureRHI::GetBindlessHandle() const
{
    return View ? View->GetBindlessHandle() : FRHIDescriptorHandle();
}

void FMetalSceneAccelerationStructureRHI::SetDebugName(const String& InName)
{
    SetLabel(InName);
}

void FMetalSceneAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = GetLabel();
}

bool FMetalSceneAccelerationStructureRHI::Build(FMetalCommandContext& Context, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    @autoreleasepool
    {
        typedef MTLIndirectAccelerationStructureInstanceDescriptor FInstanceDescriptor;

        const uint64 DescriptorBytes = sizeof(FInstanceDescriptor) * Math::Max<uint64>(BuildDesc.NumInstances, 1);

        FMetalResourceStorage InstanceStorage(GetDevice());
        void* Mapped = GetDevice()->GetStagingBufferAllocator()->Allocate(DescriptorBytes, BUFFER_ALIGNMENT, &Context.GetQueue(), InstanceStorage);
        if (!Mapped)
        {
            METAL_ERROR("Failed to allocate %llu bytes of instance descriptors", DescriptorBytes);
            return false;
        }

        FInstanceDescriptor* InstanceDescriptors = static_cast<FInstanceDescriptor*>(Mapped);

        uint32 NumInstances = 0;
        for (uint32 Index = 0; Index < BuildDesc.NumInstances; ++Index)
        {
            const FRHIGeometryAccelerationStructureInstance& Instance = BuildDesc.Instances[Index];

            FMetalGeometryAccelerationStructureRHI* MetalGeometry     = static_cast<FMetalGeometryAccelerationStructureRHI*>(Instance.Geometry);
            id<MTLAccelerationStructure>            GeometryStructure = MetalGeometry ? MetalGeometry->GetMTLAccelerationStructure() : nil;
            if (!GeometryStructure)
            {
                METAL_WARNING("TLAS build skipping instance %u with null geometry (no BLAS)", Index);
                continue;
            }

            FInstanceDescriptor& Descriptor = InstanceDescriptors[NumInstances++];
            Descriptor.accelerationStructureID         = GeometryStructure.gpuResourceID;
            Descriptor.userID                          = Instance.InstanceIndex;
            Descriptor.mask                            = Instance.Mask;
            Descriptor.intersectionFunctionTableOffset = Instance.HitGroupIndex;
            Descriptor.options                         = ConvertInstanceFlags(Instance.Flags);
            Descriptor.transformationMatrix            = ConvertTransform(Instance.Transform);
        }

        MTLInstanceAccelerationStructureDescriptor* Descriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];
        Descriptor.instanceDescriptorType         = MTLAccelerationStructureInstanceDescriptorTypeIndirect;
        Descriptor.instanceDescriptorBuffer       = InstanceStorage.GetBuffer();
        Descriptor.instanceDescriptorBufferOffset = InstanceStorage.GetResourceOffset();
        Descriptor.instanceDescriptorStride       = sizeof(FInstanceDescriptor);
        Descriptor.instanceCount                  = NumInstances;
        Descriptor.usage                          = ConvertAccelerationStructureUsage(GetFlags());

        const bool bRefit = BuildDesc.bUpdate && IsEnumFlagSet(GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate) && GetMTLAccelerationStructure() != nil && NumInstances == NumBuiltInstances;
        if (!bRefit)
        {
            const MTLSizeAndAlign SizeAndAlign = [GetDevice()->GetMTLDevice() heapAccelerationStructureSizeAndAlignWithDescriptor:Descriptor];
            if (!ReserveStorage(Context, SizeAndAlign))
            {
                return false;
            }
        }

        if (!EncodeBuild(Context, Descriptor, bRefit))
        {
            return false;
        }

        NumBuiltInstances = NumInstances;
        return true;
    }
}

void FMetalSceneAccelerationStructureRHI::OnStorageReplaced()
{
    View->RefreshBindlessHandle();
}
