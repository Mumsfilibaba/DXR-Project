#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12RayTracing.h"

FD3D12AccelerationStructure::FD3D12AccelerationStructure(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
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

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo = {};
    GetDevice()->GetD3D12Device5()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    uint64 CurrentSize = ResultBuffer ? ResultBuffer->GetWidth() : 0;
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = PreBuildInfo.ResultDataMaxSizeInBytes;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        FD3D12ResourceRef Buffer = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Initialize(D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr))
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ResultBuffer = Buffer;
        }
    }

    const uint64 RequiredSize = Math::Max(PreBuildInfo.ScratchDataSizeInBytes, PreBuildInfo.UpdateScratchDataSizeInBytes);
    CurrentSize = ScratchBuffer ? ScratchBuffer->GetWidth() : 0;
    if (CurrentSize < RequiredSize)
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = RequiredSize;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        FD3D12ResourceRef Buffer = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Initialize(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ScratchBuffer = Buffer;
        }

        CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(ScratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();

    CmdContext.GetResourceBarrierBatcher().FlushBarriers();

    FD3D12CommandList& CommandList = CmdContext.GetCommandList();
    CommandList.GetGraphicsCommandList4()->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc, 0, nullptr);

    CmdContext.GetResourceBarrierBatcher().AddUnorderedAccessBarrier(ResultBuffer.Get());
    return true;
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
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        return D3D12Resource->GetDebugName();
    }

    return FString();
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

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo = {};
    GetDevice()->GetD3D12Device5()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    uint64 CurrentSize = ResultBuffer ? ResultBuffer->GetWidth() : 0;
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = PreBuildInfo.ResultDataMaxSizeInBytes;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        FD3D12ResourceRef Buffer = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Initialize(D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr))
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ResultBuffer = Buffer;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
        SrvDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        SrvDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SrvDesc.RaytracingAccelerationStructure.Location = ResultBuffer->GetGPUVirtualAddress();

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
    CurrentSize = ScratchBuffer ? ScratchBuffer->GetWidth() : 0;
    if (CurrentSize < RequiredSize)
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = RequiredSize;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        FD3D12ResourceRef Buffer = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Initialize(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ScratchBuffer = Buffer;
        }

        CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(ScratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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

        FD3D12ResourceRef Buffer = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Initialize(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            InstanceBuffer = Buffer;
        }

        CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    CmdContext.UpdateBuffer(InstanceBuffer.Get(), FBufferRegion(0, InstanceDescs.SizeInBytes()), InstanceDescs.Data());
    CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.Inputs.InstanceDescs             = InstanceBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();

    if (BuildDesc.bUpdate)
    {
        CHECK((GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None);
        AccelerationStructureDesc.SourceAccelerationStructureData = ResultBuffer->GetGPUVirtualAddress();
    }

    CmdContext.GetResourceBarrierBatcher().FlushBarriers();

    FD3D12CommandList& CommandList = CmdContext.GetCommandList();
    CommandList.GetGraphicsCommandList4()->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc, 0, nullptr);

    CmdContext.GetResourceBarrierBatcher().AddUnorderedAccessBarrier(ResultBuffer.Get());

    Instances.Reset(BuildDesc.Instances, BuildDesc.NumInstances);
    return true;
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
        ResourceHeap,
        SamplerHeap,
        RayGenEntry,
        *RayGenLocalResources);

    CHECK(MissLocalResources != nullptr);

    FD3D12ShaderBindingTableEntry MissEntry;
    ShaderBindingTableBuilder.PopulateEntry(
        PipelineState,
        PipelineState->GetMissLocalRootSignature(),
        ResourceHeap,
        SamplerHeap,
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
            ResourceHeap,
            SamplerHeap,
            HitGroupEntries[i],
            HitGroupResources[i]);
    }

    ShaderBindingTableBuilder.CopyDescriptors();

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

        FD3D12ResourceRef Buffer = new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Initialize(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            BindingTable = Buffer;
        }

        CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(BindingTable.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    // NOTE: With resource tracking this would not be needed
    CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(BindingTable.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    CmdContext.UpdateBuffer(BindingTable.Get(), FBufferRegion(0, TableEntrySize), &RayGenEntry);
    CmdContext.UpdateBuffer(BindingTable.Get(), FBufferRegion(TableEntrySize, TableEntrySize), &MissEntry);
    CmdContext.UpdateBuffer(BindingTable.Get(), FBufferRegion(TableEntrySize * 2, NumHitGroupResources * TableEntrySize), HitGroupEntries);
    CmdContext.GetResourceBarrierBatcher().AddTransitionBarrier(BindingTable.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

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
    FD3D12Resource* D3D12Resource = GetResource();
    if (D3D12Resource)
    {
        return D3D12Resource->GetDebugName();
    }

    return FString();
}

FD3D12ShaderBindingTableBuilder::FD3D12ShaderBindingTableBuilder(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
{
    Reset();
}

void FD3D12ShaderBindingTableBuilder::PopulateEntry(
    FD3D12RayTracingPipelineStateRHI* /* PipelineState */,
    FD3D12RootSignature* /* RootSignature */,
    FD3D12OnlineDescriptorHeap* /* ResourceHeap */,
    FD3D12OnlineDescriptorHeap* /* SamplerHeap */,
    FD3D12ShaderBindingTableEntry& /* OutShaderBindingEntry */,
    const FRayTracingShaderResources& /* Resources */)
{
#if 0
    CHECK(PipelineState != nullptr);
    CHECK(RootSignature != nullptr);
    CHECK(ResourceHeap  != nullptr);
    CHECK(SamplerHeap   != nullptr);

    FMemory::Memcpy(OutShaderBindingEntry.ShaderIdentifier, PipelineState->GetShaderIdentifier(Resources.Identifier), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    if (!Resources.ConstantBuffers.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_CBV);
        CHECK(RootIndex < 4);

        uint32 NumDescriptors = Resources.ConstantBuffers.Size();
        uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUHandle(Handle);

        GPUResourceHandles[GPUResourceIndex]       = ResourceHeap->GetCPUHandle(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (FRHIBuffer* ConstantBuffer : Resources.ConstantBuffers)
        {
            FD3D12BufferRHI* D3D12ConstantBuffer = FD3D12RHI::ResourceCast(ConstantBuffer);
            ResourceHandles[CPUResourceIndex++] = D3D12ConstantBuffer->GetConstantBufferView()->GetOfflineHandle();
        }
    }
    if (!Resources.ShaderResourceViews.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_SRV);
        CHECK(RootIndex < 4);

        uint32 NumDescriptors = Resources.ShaderResourceViews.Size();
        uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUHandle(Handle);

        GPUResourceHandles[GPUResourceIndex]       = ResourceHeap->GetCPUHandle(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (FRHIShaderResourceView* ShaderResourceView : Resources.ShaderResourceViews)
        {
            FD3D12ShaderResourceViewRHI* DxShaderResourceView = FD3D12RHI::ResourceCast(ShaderResourceView);
            ResourceHandles[CPUResourceIndex++] = DxShaderResourceView->GetOfflineHandle();
        }
    }
    if (!Resources.UnorderedAccessViews.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_UAV);
        CHECK(RootIndex < 4);

        uint32 NumDescriptors = Resources.UnorderedAccessViews.Size();
        uint32 Handle = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUHandle(Handle);

        GPUResourceHandles[GPUResourceIndex] = ResourceHeap->GetCPUHandle(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (FRHIUnorderedAccessView* UnorderedAccessView : Resources.UnorderedAccessViews)
        {
            FD3D12UnorderedAccessViewRHI* DxUnorderedAccessView = FD3D12RHI::ResourceCast(UnorderedAccessView);
            ResourceHandles[CPUResourceIndex++] = DxUnorderedAccessView->GetOfflineHandle();
        }
    }
    if (!Resources.SamplerStates.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_Sampler);
        CHECK(RootIndex < 4);

        uint32 NumDescriptors = Resources.SamplerStates.Size();
        uint32 Handle = SamplerHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = SamplerHeap->GetGPUHandle(Handle);

        GPUSamplerHandles[GPUSamplerIndex] = SamplerHeap->GetCPUHandle(Handle);
        GPUSamplerHandleSizes[GPUSamplerIndex++] = NumDescriptors;

        for (FRHISamplerState* Sampler : Resources.SamplerStates)
        {
            FD3D12SamplerStateRHI* DxSampler = FD3D12RHI::ResourceCast(Sampler);
            SamplerHandles[CPUSamplerIndex++] = DxSampler->GetOfflineHandle();
        }
    }
#endif
}

void FD3D12ShaderBindingTableBuilder::CopyDescriptors()
{
    GetDevice()->GetD3D12Device()->CopyDescriptors(
        GPUResourceIndex,
        GPUResourceHandles,
        GPUResourceHandleSizes,
        CPUResourceIndex,
        ResourceHandles,
        CPUHandleSizes,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    GetDevice()->GetD3D12Device()->CopyDescriptors(
        GPUSamplerIndex,
        GPUSamplerHandles,
        GPUSamplerHandleSizes,
        CPUSamplerIndex,
        SamplerHandles,
        CPUHandleSizes,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void FD3D12ShaderBindingTableBuilder::Reset()
{
    for (uint32 i = 0; i < ARRAY_COUNT(CPUHandleSizes); i++)
    {
        CPUHandleSizes[i] = 1;
    }

    CPUResourceIndex = 0;
    CPUSamplerIndex  = 0;
    GPUResourceIndex = 0;
    GPUSamplerIndex  = 0;
}
