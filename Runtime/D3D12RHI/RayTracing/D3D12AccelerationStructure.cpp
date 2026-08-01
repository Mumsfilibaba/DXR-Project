#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/RayTracing/D3D12AccelerationStructure.h"
#include "RHI/RHIStats.h"

#if D3D12_ENABLE_OPACITY_MICROMAPS
static D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT ConvertOpacityMicromapFormat(EOpacityMicromapFormat Format)
{
    return (Format == EOpacityMicromapFormat::OC1_4State)
        ? D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_4_STATE
        : D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_2_STATE;
}
#endif

FD3D12AccelerationStructure::FD3D12AccelerationStructure(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , ResultResourceStorage(InDevice)
    , ScratchResourceStorage(InDevice)
    , TrackedASMemory(0)
{
}

FD3D12AccelerationStructure::~FD3D12AccelerationStructure()
{
    if (TrackedASMemory > 0)
    {
        STAT_SUBTRACT(STAT_RHI_AccelerationStructureMemory, TrackedASMemory);
        TrackedASMemory = 0;
    }
}

void FD3D12AccelerationStructure::UpdateAccelerationStructureMemoryStat()
{
    const uint64 NewSize = ResultResourceStorage.GetSize() + ScratchResourceStorage.GetSize();
    if (NewSize > TrackedASMemory)
    {
        STAT_ADD(STAT_RHI_AccelerationStructureMemory, NewSize - TrackedASMemory);
    }
    else if (NewSize < TrackedASMemory)
    {
        STAT_SUBTRACT(STAT_RHI_AccelerationStructureMemory, TrackedASMemory - NewSize);
    }

    TrackedASMemory = NewSize;
}

bool FD3D12AccelerationStructure::CompactInPlace(FD3D12CommandContext& CmdContext, uint64 CompactedSize)
{
#if D3D12_USE_ID3D12COMMANDLIST_4
    if (CompactedSize == 0 || !ResultResourceStorage.GetResource())
    {
        return false;
    }

    FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
    if (!Allocator)
    {
        return false;
    }

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Width              = CompactedSize;
    ResourceDesc.Height             = 1;
    ResourceDesc.DepthOrArraySize   = 1;
    ResourceDesc.MipLevels          = 1;
    ResourceDesc.Alignment          = 0;
    ResourceDesc.SampleDesc.Count   = 1;
    ResourceDesc.SampleDesc.Quality = 0;

    FD3D12ResourceStorage CompactedStorage(GetDevice());
    const bool bAllocated = Allocator->TryAllocate(
        D3D12_HEAP_TYPE_DEFAULT,
        ResourceDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        ED3D12ResourceStateMode::MultipleStates,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
        CompactedStorage);

    if (!bAllocated || CompactedStorage.GetResource() == nullptr)
    {
        return false;
    }

    CmdContext.GetBarrierBatcher().FlushBarriers(CmdContext.GetCommandList());

    FD3D12CommandList& CommandList = CmdContext.GetCommandList();
    CommandList.UpdateResidency(CompactedStorage.GetResource()->GetResidencyHandle());
    CommandList.UpdateResidency(ResultResourceStorage.GetResource()->GetResidencyHandle());

    CommandList.GetGraphicsCommandList4()->CopyRaytracingAccelerationStructure(
        CompactedStorage.GetGPUVirtualAddress(),
        ResultResourceStorage.GetGPUVirtualAddress(),
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);

    ResultResourceStorage.Swap(CompactedStorage);
    UpdateAccelerationStructureMemoryStat();
    return true;
#else
    UNREFERENCED_VARIABLE(CmdContext);
    UNREFERENCED_VARIABLE(CompactedSize);
    return false;
#endif
}

#if D3D12_ENABLE_OPACITY_MICROMAPS

FD3D12OpacityMicromapRHI::FD3D12OpacityMicromapRHI(FD3D12Device* InDevice, const FRHIOpacityMicromapDesc& InDesc)
    : FRHIOpacityMicromap(InDesc)
    , FD3D12AccelerationStructure(InDevice)
    , Format(InDesc.Format)
    , SubdivisionLevel(InDesc.SubdivisionLevel)
{
}

FD3D12OpacityMicromapRHI::~FD3D12OpacityMicromapRHI() = default;

void* FD3D12OpacityMicromapRHI::GetRHINativeResource() const
{
    FD3D12Resource* Resource = GetResource();
    return Resource ? reinterpret_cast<void*>(Resource->GetD3D12Resource()) : nullptr;
}

bool FD3D12OpacityMicromapRHI::Build(FD3D12CommandContext& CmdContext, const FRHIOpacityMicromapBuildDesc& BuildDesc)
{
#if D3D12_USE_ID3D12DEVICE_5 && D3D12_USE_ID3D12COMMANDLIST_4
    FD3D12BufferRHI* DescBuffer = FD3D12DeviceRHI::ResourceCast(BuildDesc.OMMDescriptorBuffer);
    FD3D12BufferRHI* DataBuffer = BuildDesc.OpacityMicroTriangleDataBuffer ? FD3D12DeviceRHI::ResourceCast(BuildDesc.OpacityMicroTriangleDataBuffer) : DescBuffer;

    if (!DescBuffer || !DataBuffer || BuildDesc.NumOpacityMicromaps == 0)
    {
        return false;
    }

    TArray<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY> HistogramEntries;
    if (BuildDesc.HistogramEntries.IsEmpty())
    {
        D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY& HistogramEntry = HistogramEntries.Emplace();
        HistogramEntry.Count            = BuildDesc.NumOpacityMicromaps;
        HistogramEntry.SubdivisionLevel = SubdivisionLevel;
        HistogramEntry.Format           = ConvertOpacityMicromapFormat(Format);
    }
    else
    {
        HistogramEntries.Reserve(BuildDesc.HistogramEntries.Size());

        for (const FRHIOpacityMicromapHistogramEntry& Entry : BuildDesc.HistogramEntries)
        {
            D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY& HistogramEntry = HistogramEntries.Emplace();
            HistogramEntry.Count            = Entry.Count;
            HistogramEntry.SubdivisionLevel = Entry.SubdivisionLevel;
            HistogramEntry.Format           = ConvertOpacityMicromapFormat(Entry.Format);
        }
    }

    const D3D12_GPU_VIRTUAL_ADDRESS DescAddress = DescBuffer->GetGPUVirtualAddress() + BuildDesc.OMMDescriptorBufferOffset;
    const D3D12_GPU_VIRTUAL_ADDRESS DataAddress = DataBuffer->GetGPUVirtualAddress() + BuildDesc.OpacityMicroTriangleDataBufferOffset;
    const uint32 DescStride = (BuildDesc.OMMDescriptorStrideInBytes != 0) ? BuildDesc.OMMDescriptorStrideInBytes : sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_DESC);

    D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC OMMArrayDesc = {};
    OMMArrayDesc.NumOmmHistogramEntries    = static_cast<uint32>(HistogramEntries.Size());
    OMMArrayDesc.pOmmHistogram             = HistogramEntries.Data();
    OMMArrayDesc.InputBuffer               = DataAddress;
    OMMArrayDesc.PerOmmDescs.StartAddress  = DescAddress;
    OMMArrayDesc.PerOmmDescs.StrideInBytes = DescStride;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
    Inputs.Type                      = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY;
    Inputs.Flags                     = ConvertAccelerationStructureBuildFlags(GetFlags());
    Inputs.NumDescs                  = 1;
    Inputs.DescsLayout               = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.pOpacityMicromapArrayDesc = &OMMArrayDesc;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo = {};
    GetDevice()->GetD3D12Device5()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
    if (!Allocator || PreBuildInfo.ResultDataMaxSizeInBytes == 0)
    {
        return false;
    }

    const auto AllocateBuffer = [&](uint64 Size, D3D12_RESOURCE_STATES InitialState, FD3D12ResourceStorage& Storage) -> bool
    {
        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ResourceDesc.Width              = Size;
        ResourceDesc.Height             = 1;
        ResourceDesc.DepthOrArraySize   = 1;
        ResourceDesc.MipLevels          = 1;
        ResourceDesc.SampleDesc.Count   = 1;

        const bool bAllocated = Allocator->TryAllocate(
            D3D12_HEAP_TYPE_DEFAULT, 
            ResourceDesc, 
            InitialState, 
            ED3D12ResourceStateMode::MultipleStates,
            D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BYTE_ALIGNMENT, 
            Storage);
        
        if (!bAllocated || Storage.GetResource() == nullptr)
        {
            return false;
        }

        return true;
    };

    if (!AllocateBuffer(PreBuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, ResultResourceStorage))
    {
        return false;
    }
    if (!AllocateBuffer(Math::Max(PreBuildInfo.ScratchDataSizeInBytes, uint64(1)), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, ScratchResourceStorage))
    {
        return false;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC BuildAS = {};
    BuildAS.Inputs                           = Inputs;
    BuildAS.DestAccelerationStructureData    = ResultResourceStorage.GetGPUVirtualAddress();
    BuildAS.ScratchAccelerationStructureData = ScratchResourceStorage.GetGPUVirtualAddress();

    CmdContext.TransitionTrackedResourceState(DescBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    CmdContext.TransitionTrackedResourceState(DataBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    CmdContext.TransitionTrackedResourceState(ScratchResourceStorage.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    CmdContext.GetBarrierBatcher().FlushBarriers(CmdContext.GetCommandList());
    CmdContext.GetCommandList().GetGraphicsCommandList4()->BuildRaytracingAccelerationStructure(&BuildAS, 0, nullptr);
    return true;
#else
    UNREFERENCED_VARIABLE(CmdContext);
    UNREFERENCED_VARIABLE(BuildDesc);
    return false;
#endif
}
#endif // D3D12_ENABLE_OPACITY_MICROMAPS

FD3D12GeometryAccelerationStructureRHI::FD3D12GeometryAccelerationStructureRHI(FD3D12Device* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
    : FRHIGeometryAccelerationStructure(InGeometryDesc)
    , FD3D12AccelerationStructure(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
{
    STAT_ADD(STAT_RHI_BLASCount, 1);
}

FD3D12GeometryAccelerationStructureRHI::~FD3D12GeometryAccelerationStructureRHI()
{
    STAT_SUBTRACT(STAT_RHI_BLASCount, 1);
}

void* FD3D12GeometryAccelerationStructureRHI::GetRHINativeResource() const
{
    FD3D12Resource* Resource = GetResource();
    return Resource ? reinterpret_cast<void*>(Resource->GetD3D12Resource()) : nullptr;
}

bool FD3D12GeometryAccelerationStructureRHI::Build(FD3D12CommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    VertexBuffer = MakeSharedRef<FD3D12BufferRHI>(BuildDesc.VertexBuffer);
    IndexBuffer  = MakeSharedRef<FD3D12BufferRHI>(BuildDesc.IndexBuffer);

    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc = {};
    GeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress  = VertexBuffer->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes = VertexBuffer->GetDesc().Stride;
    GeometryDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
    GeometryDesc.Triangles.VertexCount                = BuildDesc.NumVertices;
    GeometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    if (IndexBuffer)
    {
        GeometryDesc.Triangles.IndexFormat = ConvertIndexFormat(BuildDesc.IndexFormat);
        GeometryDesc.Triangles.IndexBuffer = IndexBuffer->GetGPUVirtualAddress();
        GeometryDesc.Triangles.IndexCount  = BuildDesc.NumIndices;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
    Inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs       = 1;
    Inputs.pGeometryDescs = &GeometryDesc;
    Inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    Inputs.Flags          = ConvertAccelerationStructureBuildFlags(GetFlags());

    if (BuildDesc.bUpdate)
    {
        Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }

#if D3D12_USE_ID3D12DEVICE_5
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo = {};
    GetDevice()->GetD3D12Device5()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    uint64 CurrentSize = ResultResourceStorage.GetSize();
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
        if (!Allocator)
        {
            return false;
        }

        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ResourceDesc.Width              = PreBuildInfo.ResultDataMaxSizeInBytes;
        ResourceDesc.Height             = 1;
        ResourceDesc.DepthOrArraySize   = 1;
        ResourceDesc.MipLevels          = 1;
        ResourceDesc.Alignment          = 0;
        ResourceDesc.SampleDesc.Count   = 1;
        ResourceDesc.SampleDesc.Quality = 0;

        bool bResult = Allocator->TryAllocate(
            D3D12_HEAP_TYPE_DEFAULT, 
            ResourceDesc, 
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
            ED3D12ResourceStateMode::MultipleStates, 
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, 
            ResultResourceStorage);

        if (!bResult || ResultResourceStorage.GetResource() == nullptr)
        {
            return false;
        }
    }

    const uint64 RequiredSize = Math::Max(PreBuildInfo.ScratchDataSizeInBytes, PreBuildInfo.UpdateScratchDataSizeInBytes);
    CurrentSize = ScratchResourceStorage.GetSize();
    
    if (CurrentSize < RequiredSize)
    {
        FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
        if (!Allocator)
        {
            return false;
        }

        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ResourceDesc.Width              = RequiredSize;
        ResourceDesc.Height             = 1;
        ResourceDesc.DepthOrArraySize   = 1;
        ResourceDesc.MipLevels          = 1;
        ResourceDesc.Alignment          = 0;
        ResourceDesc.SampleDesc.Count   = 1;
        ResourceDesc.SampleDesc.Quality = 0;

        bool bScratchResult = Allocator->TryAllocate(
            D3D12_HEAP_TYPE_DEFAULT, 
            ResourceDesc, 
            D3D12_RESOURCE_STATE_COMMON, 
            ED3D12ResourceStateMode::MultipleStates, 
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, 
            ScratchResourceStorage);

        if (!bScratchResult || ScratchResourceStorage.GetResource() == nullptr)
        {
            return false;
        }
    }

    UpdateAccelerationStructureMemoryStat();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultResourceStorage.GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchResourceStorage.GetGPUVirtualAddress();

    CmdContext.TransitionTrackedResourceState(VertexBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    CmdContext.TransitionTrackedResourceState(IndexBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    CmdContext.TransitionTrackedResourceState(ScratchResourceStorage.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    CmdContext.GetBarrierBatcher().FlushBarriers(CmdContext.GetCommandList());

    FD3D12CommandList& CommandList = CmdContext.GetCommandList();
    CommandList.UpdateResidency(ResultResourceStorage.GetResource()->GetResidencyHandle());
    CommandList.UpdateResidency(ScratchResourceStorage.GetResource()->GetResidencyHandle());

    if (VertexBuffer)
    {
        CommandList.UpdateResidency(VertexBuffer->GetResource()->GetResidencyHandle());
    }

    if (IndexBuffer)
    {
        CommandList.UpdateResidency(IndexBuffer->GetResource()->GetResidencyHandle());
    }

#if D3D12_USE_ID3D12COMMANDLIST_4
    CommandList.GetGraphicsCommandList4()->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc, 0, nullptr);
    STAT_ADD(STAT_RHI_AccelerationStructureBuilds, 1);
#endif

    CmdContext.GetBarrierBatcher().AddUnorderedAccessBarrier(ResultResourceStorage.GetResource());
    return true;
#else
    D3D12_ERROR_CRITICAL("[D3D12RayTracingGeometry]: ID3D12Device5 is required for ray tracing");
    return false;
#endif
}

void FD3D12GeometryAccelerationStructureRHI::SetDebugName(const String& InName)
{
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        D3D12Resource->SetDebugName(InName);
    }
}

void FD3D12GeometryAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
    if (FD3D12Resource* D3D12Resource = GetResource())
    {
        D3D12Resource->GetDebugName(OutDebugName);
    }
    else
    {
        OutDebugName.Clear();
    }
}

FD3D12SceneAccelerationStructureRHI::FD3D12SceneAccelerationStructureRHI(FD3D12Device* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc)
    : FRHISceneAccelerationStructure(InSceneDesc)
    , FD3D12AccelerationStructure(InDevice)
    , Instances()
    , View(nullptr)
    , InstanceBuffer(nullptr)
{
    STAT_ADD(STAT_RHI_TLASCount, 1);
}

FD3D12SceneAccelerationStructureRHI::~FD3D12SceneAccelerationStructureRHI()
{
    STAT_SUBTRACT(STAT_RHI_TLASCount, 1);
}

void* FD3D12SceneAccelerationStructureRHI::GetRHINativeResource() const
{
    FD3D12Resource* Resource = GetResource();
    return Resource ? reinterpret_cast<void*>(Resource->GetD3D12Resource()) : nullptr;
}

FRHIShaderResourceView* FD3D12SceneAccelerationStructureRHI::GetShaderResourceView() const
{
    return View.Get();
}

FRHIDescriptorHandle FD3D12SceneAccelerationStructureRHI::GetBindlessHandle() const
{
    return View ? View->GetBindlessHandle() : FRHIDescriptorHandle();
}

bool FD3D12SceneAccelerationStructureRHI::Build(FD3D12CommandContext& CmdContext, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
    Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs    = BuildDesc.NumInstances;
    Inputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    Inputs.Flags       = ConvertAccelerationStructureBuildFlags(GetFlags());
    
    if (BuildDesc.bUpdate)
    {
        CHECK((GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None);
        Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }

#if D3D12_USE_ID3D12DEVICE_5
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo = {};
    GetDevice()->GetD3D12Device5()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    uint64 CurrentSize = ResultResourceStorage.GetSize();
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
        if (!Allocator)
        {
            return false;
        }

        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ResourceDesc.Width              = PreBuildInfo.ResultDataMaxSizeInBytes;
        ResourceDesc.Height             = 1;
        ResourceDesc.DepthOrArraySize   = 1;
        ResourceDesc.MipLevels          = 1;
        ResourceDesc.Alignment          = 0;
        ResourceDesc.SampleDesc.Count   = 1;
        ResourceDesc.SampleDesc.Quality = 0;

        bool bResult = Allocator->TryAllocate(
            D3D12_HEAP_TYPE_DEFAULT, 
            ResourceDesc, 
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
            ED3D12ResourceStateMode::MultipleStates, 
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, 
            ResultResourceStorage);

        if (!bResult || ResultResourceStorage.GetResource() == nullptr)
        {
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
        SrvDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        SrvDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SrvDesc.RaytracingAccelerationStructure.Location = ResultResourceStorage.GetGPUVirtualAddress();

        View = new FD3D12ShaderResourceViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this, FRHIShaderResourceViewDesc::CreateAccelerationStructure());
        if (!View->Initialize(nullptr, SrvDesc))
        {
            return false;
        }
    }

    const uint64 RequiredSize = Math::Max(PreBuildInfo.ScratchDataSizeInBytes, PreBuildInfo.UpdateScratchDataSizeInBytes);
    CurrentSize = ScratchResourceStorage.GetSize();

    if (CurrentSize < RequiredSize)
    {
        FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
        if (!Allocator)
        {
            return false;
        }

        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ResourceDesc.Width              = RequiredSize;
        ResourceDesc.Height             = 1;
        ResourceDesc.DepthOrArraySize   = 1;
        ResourceDesc.MipLevels          = 1;
        ResourceDesc.Alignment          = 0;
        ResourceDesc.SampleDesc.Count   = 1;
        ResourceDesc.SampleDesc.Quality = 0;

        bool bResult = Allocator->TryAllocate(
            D3D12_HEAP_TYPE_DEFAULT,
            ResourceDesc, 
            D3D12_RESOURCE_STATE_COMMON, 
            ED3D12ResourceStateMode::MultipleStates, 
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, 
            ScratchResourceStorage);

        if (!bResult || ScratchResourceStorage.GetResource() == nullptr)
        {
            return false;
        }
    }

    TArray<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs;
    InstanceDescs.Reserve(BuildDesc.NumInstances);

    for (uint32 Instance = 0; Instance < BuildDesc.NumInstances; Instance++)
    {
        FD3D12GeometryAccelerationStructureRHI* D3D12Geometry = FD3D12DeviceRHI::ResourceCast(BuildDesc.Instances[Instance].Geometry);
        if (!D3D12Geometry)
        {
            D3D12_WARNING("TLAS build skipping instance %u with null geometry (no BLAS)", Instance);
            continue;
        }

        D3D12_RAYTRACING_INSTANCE_DESC& Desc = InstanceDescs.Emplace();
        Memory::Memcpy(&Desc.Transform, &BuildDesc.Instances[Instance].Transform, sizeof(Matrix3x4));

        Desc.AccelerationStructure               = D3D12Geometry->GetGPUVirtualAddress();
        Desc.InstanceID                          = BuildDesc.Instances[Instance].InstanceIndex;
        Desc.Flags                               = ConvertRayTracingInstanceFlags(BuildDesc.Instances[Instance].Flags);
        Desc.InstanceMask                        = BuildDesc.Instances[Instance].Mask;
        Desc.InstanceContributionToHitGroupIndex = BuildDesc.Instances[Instance].HitGroupIndex;
    }

    CurrentSize = InstanceBuffer ? InstanceBuffer->GetWidth() : 0;
    if (CurrentSize < InstanceDescs.SizeInBytes())
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = InstanceDescs.SizeInBytes();
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        FD3D12ResourceRef Buffer;
        if (!GetDevice()->CreateCommittedResource(Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, nullptr, Buffer))
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            InstanceBuffer = Buffer;
        }
    }

    // UpdateBuffer takes the instance buffer to COPY_DEST itself, the build then reads it as a shader resource.
    CmdContext.UpdateBuffer(InstanceBuffer.Get(), FBufferRegion(0, InstanceDescs.SizeInBytes()), InstanceDescs.Data());
    CmdContext.TransitionTrackedResourceState(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    UpdateAccelerationStructureMemoryStat();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.Inputs.NumDescs                  = uint32(InstanceDescs.Size());
    AccelerationStructureDesc.Inputs.InstanceDescs             = InstanceBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultResourceStorage.GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchResourceStorage.GetGPUVirtualAddress();

    if (BuildDesc.bUpdate)
    {
        CHECK((GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None);
        AccelerationStructureDesc.SourceAccelerationStructureData = ResultResourceStorage.GetGPUVirtualAddress();
    }

    CmdContext.GetBarrierBatcher().FlushBarriers(CmdContext.GetCommandList());

    FD3D12CommandList& CommandList = CmdContext.GetCommandList();
    CommandList.UpdateResidency(ResultResourceStorage.GetResource()->GetResidencyHandle());
    CommandList.UpdateResidency(ScratchResourceStorage.GetResource()->GetResidencyHandle());
    CommandList.UpdateResidency(InstanceBuffer->GetResidencyHandle());

#if D3D12_USE_ID3D12COMMANDLIST_4
    CommandList.GetGraphicsCommandList4()->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc, 0, nullptr);
    STAT_ADD(STAT_RHI_AccelerationStructureBuilds, 1);
#endif

    CmdContext.GetBarrierBatcher().AddUnorderedAccessBarrier(ResultResourceStorage.GetResource());

    Instances.Reset(BuildDesc.Instances, BuildDesc.NumInstances);
    return true;
#else
    D3D12_ERROR_CRITICAL("[D3D12RayTracingScene]: ID3D12Device5 is required for ray tracing");
    return false;
#endif
}

void FD3D12SceneAccelerationStructureRHI::SetDebugName(const String& InName)
{
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        D3D12Resource->SetDebugName(InName);
    }
}

void FD3D12SceneAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
    if (FD3D12Resource* D3D12Resource = GetResource())
    {
        D3D12Resource->GetDebugName(OutDebugName);
    }
    else
    {
        OutDebugName.Clear();
    }
}
