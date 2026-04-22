#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12RayTracing.h"

FD3D12AccelerationStructure::FD3D12AccelerationStructure(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , ResultResourceStorage(InDevice)
    , ScratchResourceStorage(InDevice)
{
}

FD3D12GeometryAccelerationStructureRHI::FD3D12GeometryAccelerationStructureRHI(FD3D12Device* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
    : FRHIGeometryAccelerationStructure(InGeometryDesc)
    , FD3D12AccelerationStructure(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
{
}

bool FD3D12GeometryAccelerationStructureRHI::Build(FD3D12CommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    VertexBuffer = MakeSharedRef<FD3D12BufferRHI>(BuildDesc.VertexBuffer);
    IndexBuffer  = MakeSharedRef<FD3D12BufferRHI>(BuildDesc.IndexBuffer);

    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc = {};
    GeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress  = VertexBuffer->GetResource()->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes = VertexBuffer->GetDesc().Stride;
    GeometryDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
    GeometryDesc.Triangles.VertexCount                = BuildDesc.NumVertices;
    GeometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    if (IndexBuffer)
    {
        GeometryDesc.Triangles.IndexFormat = ConvertIndexFormat(BuildDesc.IndexFormat);
        GeometryDesc.Triangles.IndexBuffer = IndexBuffer->GetResource()->GetGPUVirtualAddress();
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

        if (!Allocator->TryAllocate(D3D12_HEAP_TYPE_DEFAULT, ResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, ED3D12ResourceStateMode::MultipleStates, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ResultResourceStorage) || ResultResourceStorage.GetResource() == nullptr)
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

        if (!Allocator->TryAllocate(D3D12_HEAP_TYPE_DEFAULT, ResourceDesc, D3D12_RESOURCE_STATE_COMMON, ED3D12ResourceStateMode::MultipleStates, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ScratchResourceStorage) || ScratchResourceStorage.GetResource() == nullptr)
        {
            return false;
        }

        CmdContext.GetBarrierBatcher().AddTransitionBarrier(ScratchResourceStorage.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultResourceStorage.GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchResourceStorage.GetGPUVirtualAddress();

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
#endif

    CmdContext.GetBarrierBatcher().AddUnorderedAccessBarrier(ResultResourceStorage.GetResource());
    return true;
#else
    D3D12_ERROR_CRITICAL("[D3D12RayTracingGeometry]: ID3D12Device5 is required for ray tracing");
    return false;
#endif
}

void FD3D12GeometryAccelerationStructureRHI::SetDebugName(const FString& InName)
{
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        D3D12Resource->SetDebugName(InName);
    }
}

FString FD3D12GeometryAccelerationStructureRHI::GetDebugName() const
{
    FString DebugName;
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        D3D12Resource->GetDebugName(DebugName);
    }
    return DebugName;
}

FD3D12SceneAccelerationStructureRHI::FD3D12SceneAccelerationStructureRHI(FD3D12Device* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc)
    : FRHISceneAccelerationStructure(InSceneDesc)
    , FD3D12AccelerationStructure(InDevice)
    , InstanceBuffer(nullptr)
    , BindingTable(nullptr)
    , BindingTableStride(0)
    , NumHitGroups(0)
    , View(nullptr)
    , Instances()
    , ShaderBindingTableBuilder(InDevice)
    , BindingTableHeaps{ nullptr, nullptr }
{
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

        if (!Allocator->TryAllocate(D3D12_HEAP_TYPE_DEFAULT, ResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, ED3D12ResourceStateMode::MultipleStates, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ResultResourceStorage) || ResultResourceStorage.GetResource() == nullptr)
        {
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
        SrvDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        SrvDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SrvDesc.RaytracingAccelerationStructure.Location = ResultResourceStorage.GetGPUVirtualAddress();

        View = new FD3D12ShaderResourceViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), this);
        if (!View->AllocateHandle())
        {
            return false;
        }

        if (!View->CreateView(nullptr, SrvDesc))
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

        if (!Allocator->TryAllocate(D3D12_HEAP_TYPE_DEFAULT, ResourceDesc, D3D12_RESOURCE_STATE_COMMON, ED3D12ResourceStateMode::MultipleStates, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ScratchResourceStorage) || ScratchResourceStorage.GetResource() == nullptr)
        {
            return false;
        }

        CmdContext.GetBarrierBatcher().AddTransitionBarrier(ScratchResourceStorage.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    TArray<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs(BuildDesc.NumInstances);
    for (int32 Instance = 0; Instance < InstanceDescs.Size(); Instance++)
    {
        FD3D12GeometryAccelerationStructureRHI* D3D12Geometry = FD3D12RHI::ResourceCast(BuildDesc.Instances[Instance].Geometry);
        FMemory::Memcpy(&InstanceDescs[Instance].Transform, &BuildDesc.Instances[Instance].Transform, sizeof(FMatrix3x4));

        InstanceDescs[Instance].AccelerationStructure               = D3D12Geometry->GetGPUVirtualAddress();
        InstanceDescs[Instance].InstanceID                          = BuildDesc.Instances[Instance].InstanceIndex;
        InstanceDescs[Instance].Flags                               = ConvertRayTracingInstanceFlags(BuildDesc.Instances[Instance].Flags);
        InstanceDescs[Instance].InstanceMask                        = BuildDesc.Instances[Instance].Mask;
        InstanceDescs[Instance].InstanceContributionToHitGroupIndex = BuildDesc.Instances[Instance].HitGroupIndex;
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

        CmdContext.GetBarrierBatcher().AddTransitionBarrier(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    CmdContext.GetBarrierBatcher().AddTransitionBarrier(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    CmdContext.UpdateBuffer(InstanceBuffer.Get(), FBufferRegion(0, InstanceDescs.SizeInBytes()), InstanceDescs.Data());
    CmdContext.GetBarrierBatcher().AddTransitionBarrier(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
    AccelerationStructureDesc.Inputs                           = Inputs;
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
#endif

    CmdContext.GetBarrierBatcher().AddUnorderedAccessBarrier(ResultResourceStorage.GetResource());

    Instances.Reset(BuildDesc.Instances, BuildDesc.NumInstances);
    return true;
#else
    D3D12_ERROR_CRITICAL("[D3D12RayTracingScene]: ID3D12Device5 is required for ray tracing");
    return false;
#endif
}

bool FD3D12SceneAccelerationStructureRHI::BuildBindingTable(
    FD3D12CommandContext& CmdContext,
    FD3D12RayTracingPipelineStateRHI* PipelineState,
    FD3D12OnlineDescriptorHeap* ResourceHeap,
    FD3D12OnlineDescriptorHeap* SamplerHeap,
    const FRayTracingShaderResources* RayGenLocalResources,
    const FRayTracingShaderResources* MissLocalResources,
    const FRayTracingShaderResources* HitGroupResources,
    uint32 NumHitGroupResources)
{
    CHECK(ResourceHeap         != nullptr);
    CHECK(SamplerHeap          != nullptr);
    CHECK(PipelineState        != nullptr);
    CHECK(RayGenLocalResources != nullptr);

    FD3D12ShaderBindingTableEntry RayGenEntry;
    ShaderBindingTableBuilder.PopulateEntry(
        PipelineState,
        PipelineState->GetRayGenLocalRootSignature(),
        RayGenEntry,
        *RayGenLocalResources);

    CHECK(MissLocalResources != nullptr);

    FD3D12ShaderBindingTableEntry MissEntry;
    ShaderBindingTableBuilder.PopulateEntry(
        PipelineState,
        PipelineState->GetMissLocalRootSignature(),
        MissEntry,
        *MissLocalResources);

    CHECK(HitGroupResources != nullptr);
    CHECK(NumHitGroupResources <= D3D12_MAX_HIT_GROUPS);

    FD3D12ShaderBindingTableEntry HitGroupEntries[D3D12_MAX_HIT_GROUPS];
    for (uint32 i = 0; i < NumHitGroupResources; i++)
    {
        ShaderBindingTableBuilder.PopulateEntry(
            PipelineState,
            PipelineState->GetHitLocalRootSignature(),
            HitGroupEntries[i],
            HitGroupResources[i]);
    }

    // TODO: More dynamic size of binding table
    uint32 TableEntrySize   = sizeof(FD3D12ShaderBindingTableEntry);
    uint64 BindingTableSize = TableEntrySize + TableEntrySize + (TableEntrySize * NumHitGroupResources);

    uint64 CurrentSize = BindingTable ? BindingTable->GetWidth() : 0;
    if (CurrentSize < BindingTableSize)
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = BindingTableSize;
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
            BindingTable = Buffer;
        }

        CmdContext.GetBarrierBatcher().AddTransitionBarrier(BindingTable.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    // NOTE: With resource tracking this would not be needed
    CmdContext.GetBarrierBatcher().AddTransitionBarrier(BindingTable.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    CmdContext.UpdateBuffer(BindingTable.Get(), FBufferRegion(0, TableEntrySize), &RayGenEntry);
    CmdContext.UpdateBuffer(BindingTable.Get(), FBufferRegion(TableEntrySize, TableEntrySize), &MissEntry);
    CmdContext.UpdateBuffer(BindingTable.Get(), FBufferRegion(TableEntrySize * 2, NumHitGroupResources * TableEntrySize), HitGroupEntries);
    CmdContext.GetBarrierBatcher().AddTransitionBarrier(BindingTable.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    ShaderBindingTableBuilder.Reset();
#if 0
    BindingTableHeaps[0] = ResourceHeap->GetD3D12Heap();
    BindingTableHeaps[1] = SamplerHeap->GetD3D12Heap();
#endif

    BindingTableStride = sizeof(FD3D12ShaderBindingTableEntry);
    NumHitGroups = NumHitGroupResources;

    return true;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE FD3D12SceneAccelerationStructureRHI::GetHitGroupTable() const
{
    CHECK(BindingTable != nullptr);
    CHECK(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    uint64 AddressOffset      = BindingTableStride * 2;
    uint64 SizeInBytes        = (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + AddressOffset, SizeInBytes, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE FD3D12SceneAccelerationStructureRHI::GetRayGenShaderRecord() const
{
    CHECK(BindingTable != nullptr);
    CHECK(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    return { BindingTableAdress, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE FD3D12SceneAccelerationStructureRHI::GetMissShaderTable() const
{
    CHECK(BindingTable != nullptr);
    CHECK(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    uint64 AddressOffset      = BindingTableStride;
    return { BindingTableAdress + AddressOffset, BindingTableStride, BindingTableStride };
}

void FD3D12SceneAccelerationStructureRHI::SetDebugName(const FString& InName)
{
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        D3D12Resource->SetDebugName(InName);
    }
}

FString FD3D12SceneAccelerationStructureRHI::GetDebugName() const
{
    FString DebugName;
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        D3D12Resource->GetDebugName(DebugName);
    }
    return DebugName;
}

FD3D12ShaderBindingTableBuilder::FD3D12ShaderBindingTableBuilder(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
{
    Reset();
}

void FD3D12ShaderBindingTableBuilder::PopulateEntry(
    FD3D12RayTracingPipelineStateRHI* PipelineState,
    FD3D12RootSignature* RootSignature,
    FD3D12ShaderBindingTableEntry& OutShaderBindingEntry,
    const FRayTracingShaderResources& Resources)
{
    CHECK(PipelineState != nullptr);
    CHECK(RootSignature != nullptr);

    FMemory::Memcpy(OutShaderBindingEntry.ShaderIdentifier, PipelineState->GetShaderIdentifier(Resources.Identifier), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    const FD3D12ShaderStage& Stage = RootSignature->GetShaderStage(ShaderVisibility_All);

    for (int32 i = 0; i < Resources.ConstantBuffers.Size(); i++)
    {
        const int8 ParamIndex = Stage.GetRootDescriptorParameterIndex(ResourceType_CBV, static_cast<uint16>(i));
        if (ParamIndex >= 0 && ParamIndex < D3D12_MAX_LOCAL_ROOT_DESCRIPTORS)
        {
            FD3D12BufferRHI* Buffer = FD3D12RHI::ResourceCast(Resources.ConstantBuffers[i]);
            OutShaderBindingEntry.RootDescriptors[ParamIndex] = Buffer ? Buffer->GetGPUVirtualAddress() : 0;
        }
    }

    for (int32 i = 0; i < Resources.ShaderResourceViews.Size(); i++)
    {
        const int8 ParamIndex = Stage.GetRootDescriptorParameterIndex(ResourceType_SRV, static_cast<uint16>(i));
        if (ParamIndex >= 0 && ParamIndex < D3D12_MAX_LOCAL_ROOT_DESCRIPTORS)
        {
            FD3D12ShaderResourceViewRHI* SRV = FD3D12RHI::ResourceCast(Resources.ShaderResourceViews[i]);
            const FD3D12Resource* Resource = SRV ? SRV->GetViewResource() : nullptr;
            OutShaderBindingEntry.RootDescriptors[ParamIndex] = Resource ? Resource->GetGPUVirtualAddress() : 0;
        }
    }

    for (int32 i = 0; i < Resources.UnorderedAccessViews.Size(); i++)
    {
        const int8 ParamIndex = Stage.GetRootDescriptorParameterIndex(ResourceType_UAV, static_cast<uint16>(i));
        if (ParamIndex >= 0 && ParamIndex < D3D12_MAX_LOCAL_ROOT_DESCRIPTORS)
        {
            FD3D12UnorderedAccessViewRHI* UAV = FD3D12RHI::ResourceCast(Resources.UnorderedAccessViews[i]);
            const FD3D12Resource* Resource = UAV ? UAV->GetViewResource() : nullptr;
            OutShaderBindingEntry.RootDescriptors[ParamIndex] = Resource ? Resource->GetGPUVirtualAddress() : 0;
        }
    }
}

void FD3D12ShaderBindingTableBuilder::Reset()
{
}
