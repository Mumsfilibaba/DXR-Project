#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12RHIInstance.h"
#include "D3D12RHIRayTracing.h"

#include "RHI/RHIModule.h"

#include "Engine/Assets/MeshFactory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIRayTracingGeometry

CD3D12RHIRayTracingGeometry::CD3D12RHIRayTracingGeometry(CD3D12Device* InDevice, uint32 InFlags)
    : CRHIRayTracingGeometry(InFlags)
    , CD3D12DeviceChild(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
{
}

bool CD3D12RHIRayTracingGeometry::Build(CD3D12RHICommandContext& CmdContext, bool Update)
{
    Assert(VertexBuffer != nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc;
    CMemory::Memzero(&GeometryDesc);

    GeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress  = VertexBuffer->GetResource()->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes = VertexBuffer->GetStride();
    GeometryDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
    GeometryDesc.Triangles.VertexCount                = VertexBuffer->GetNumVertices();
    GeometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    if (IndexBuffer)
    {
        ERHIIndexFormat IndexFormat        = IndexBuffer->GetFormat();
        GeometryDesc.Triangles.IndexFormat = IndexFormat == ERHIIndexFormat::uint32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        GeometryDesc.Triangles.IndexBuffer = IndexBuffer->GetResource()->GetGPUVirtualAddress();
        GeometryDesc.Triangles.IndexCount  = IndexBuffer->GetNumIndicies();
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    CMemory::Memzero(&Inputs);

    Inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs       = 1;
    Inputs.pGeometryDescs = &GeometryDesc;
    Inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    Inputs.Flags          = ConvertAccelerationStructureBuildFlags(GetFlags());
    if (Update)
    {
        Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo;
    CMemory::Memzero(&PreBuildInfo);
    GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    uint64 CurrentSize = ResultBuffer ? ResultBuffer->GetWidth() : 0;
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        D3D12_RESOURCE_DESC Desc;
        CMemory::Memzero(&Desc);

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

        TSharedRef<CD3D12Resource> Buffer = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr))
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ResultBuffer = Buffer;
        }
    }

    uint64 RequiredSize = NMath::Max(PreBuildInfo.ScratchDataSizeInBytes, PreBuildInfo.UpdateScratchDataSizeInBytes);
    CurrentSize = ScratchBuffer ? ScratchBuffer->GetWidth() : 0;
    if (CurrentSize < RequiredSize)
    {
        D3D12_RESOURCE_DESC Desc;
        CMemory::Memzero(&Desc);

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

        TSharedRef<CD3D12Resource> Buffer = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ScratchBuffer = Buffer;
        }

        CmdContext.TransitionResource(ScratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc;
    CMemory::Memzero(&AccelerationStructureDesc);

    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();

    CmdContext.FlushResourceBarriers();

    CD3D12CommandList& CmdList = CmdContext.GetCommandList();
    CmdList.BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    CmdContext.UnorderedAccessBarrier(ResultBuffer.Get());

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIRayTracingScene

CD3D12RHIRayTracingScene::CD3D12RHIRayTracingScene(CD3D12Device* InDevice, uint32 InFlags)
    : CRHIRayTracingScene(InFlags)
    , CD3D12DeviceChild(InDevice)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
    , InstanceBuffer(nullptr)
    , BindingTable(nullptr)
    , BindingTableStride(0)
    , NumHitGroups(0)
    , View(nullptr)
    , Instances()
    , ShaderBindingTableBuilder(InDevice)
{
}

bool CD3D12RHIRayTracingScene::Build(CD3D12RHICommandContext& CmdContext, const SRayTracingGeometryInstance* InInstances, uint32 NumInstances, bool Update)
{
    Assert(InInstances != nullptr && NumInstances != 0);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    CMemory::Memzero(&Inputs);

    Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs    = NumInstances;
    Inputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    Inputs.Flags       = ConvertAccelerationStructureBuildFlags(GetFlags());
    
    if (Update)
    {
        Assert(GetFlags() & RayTracingStructureBuildFlag_AllowUpdate);
        Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo;
    CMemory::Memzero(&PreBuildInfo);
    GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    uint64 CurrentSize = ResultBuffer ? ResultBuffer->GetWidth() : 0;
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        D3D12_RESOURCE_DESC Desc;
        CMemory::Memzero(&Desc);

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

        TSharedRef<CD3D12Resource> Buffer = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr))
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ResultBuffer = Buffer;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
        CMemory::Memzero(&SrvDesc);

        SrvDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        SrvDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SrvDesc.RaytracingAccelerationStructure.Location = ResultBuffer->GetGPUVirtualAddress();

        View = dbg_new CD3D12RHIShaderResourceView(GetDevice(), GD3D12RHIInstance->GetResourceOfflineDescriptorHeap());
        if (!View->AllocateHandle())
        {
            return false;
        }

        if (!View->CreateView(nullptr, SrvDesc))
        {
            return false;
        }
    }

    uint64 RequiredSize = NMath::Max(PreBuildInfo.ScratchDataSizeInBytes, PreBuildInfo.UpdateScratchDataSizeInBytes);
    CurrentSize = ScratchBuffer ? ScratchBuffer->GetWidth() : 0;
    if (CurrentSize < RequiredSize)
    {
        D3D12_RESOURCE_DESC Desc;
        CMemory::Memzero(&Desc);

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

        TSharedRef<CD3D12Resource> Buffer = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ScratchBuffer = Buffer;
        }

        CmdContext.TransitionResource(ScratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    TArray<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs(NumInstances);
    for (int32 i = 0; i < InstanceDescs.Size(); i++)
    {
        CD3D12RHIRayTracingGeometry* DxGeometry = static_cast<CD3D12RHIRayTracingGeometry*>(InInstances[i].Instance.Get());
        CMemory::Memcpy(&InstanceDescs[i].Transform, &InInstances[i].Transform, sizeof(CMatrix3x4));

        InstanceDescs[i].AccelerationStructure               = DxGeometry->GetGPUVirtualAddress();
        InstanceDescs[i].InstanceID                          = InInstances[i].InstanceIndex;
        InstanceDescs[i].Flags                               = ConvertRayTracingInstanceFlags(InInstances[i].Flags);
        InstanceDescs[i].InstanceMask                        = InInstances[i].Mask;
        InstanceDescs[i].InstanceContributionToHitGroupIndex = InInstances[i].HitGroupIndex;
    }

    CurrentSize = InstanceBuffer ? InstanceBuffer->GetWidth() : 0;
    if (CurrentSize < InstanceDescs.SizeInBytes())
    {
        D3D12_RESOURCE_DESC Desc;
        CMemory::Memzero(&Desc);

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

        TSharedRef<CD3D12Resource> Buffer = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            InstanceBuffer = Buffer;
        }

        CmdContext.TransitionResource(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    CmdContext.TransitionResource(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    CmdContext.UpdateBuffer(InstanceBuffer.Get(), 0, InstanceDescs.SizeInBytes(), InstanceDescs.Data());
    CmdContext.TransitionResource(InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc;
    CMemory::Memzero(&AccelerationStructureDesc);

    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.Inputs.InstanceDescs             = InstanceBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
    if (Update)
    {
        Assert(GetFlags() & RayTracingStructureBuildFlag_AllowUpdate);
        AccelerationStructureDesc.SourceAccelerationStructureData = ResultBuffer->GetGPUVirtualAddress();
    }

    CmdContext.FlushResourceBarriers();

    CD3D12CommandList& CmdList = CmdContext.GetCommandList();
    CmdList.BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    CmdContext.UnorderedAccessBarrier(ResultBuffer.Get());

    // Copy the instances
    Instances = TArray<SRayTracingGeometryInstance>(InInstances, NumInstances);
    return true;
}

bool CD3D12RHIRayTracingScene::BuildBindingTable( CD3D12RHICommandContext& CmdContext
                                                , CD3D12RHIRayTracingPipelineState* PipelineState
                                                , CD3D12OnlineDescriptorHeap* ResourceHeap
                                                , CD3D12OnlineDescriptorHeap* SamplerHeap
                                                , const SRayTracingShaderResources* RayGenLocalResources
                                                , const SRayTracingShaderResources* MissLocalResources
                                                , const SRayTracingShaderResources* HitGroupResources
                                                , uint32 NumHitGroupResources)
{
    Assert(ResourceHeap != nullptr);
    Assert(SamplerHeap != nullptr);
    Assert(PipelineState != nullptr);
    Assert(RayGenLocalResources != nullptr);

    SD3D12ShaderBindingTableEntry RayGenEntry;
    ShaderBindingTableBuilder.PopulateEntry(
        PipelineState,
        PipelineState->GetRayGenLocalRootSignature(),
        ResourceHeap,
        SamplerHeap,
        RayGenEntry,
        *RayGenLocalResources);

    Assert(MissLocalResources != nullptr);

    SD3D12ShaderBindingTableEntry MissEntry;
    ShaderBindingTableBuilder.PopulateEntry(
        PipelineState,
        PipelineState->GetMissLocalRootSignature(),
        ResourceHeap,
        SamplerHeap,
        MissEntry,
        *MissLocalResources);

    Assert(HitGroupResources != nullptr);
    Assert(NumHitGroupResources <= D3D12_MAX_HIT_GROUPS);

    SD3D12ShaderBindingTableEntry HitGroupEntries[D3D12_MAX_HIT_GROUPS];
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
    uint32 TableEntrySize = sizeof(SD3D12ShaderBindingTableEntry);
    uint64 BindingTableSize = TableEntrySize + TableEntrySize + (TableEntrySize * NumHitGroupResources);

    uint64 CurrentSize = BindingTable ? BindingTable->GetWidth() : 0;
    if (CurrentSize < BindingTableSize)
    {
        D3D12_RESOURCE_DESC Desc;
        CMemory::Memzero(&Desc);

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

        TSharedRef<CD3D12Resource> Buffer = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BindingTable = Buffer;
            BindingTable->SetName(GetName() + " BindingTable");
        }

        CmdContext.TransitionResource(BindingTable.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    // NOTE: With resource tracking this would not be needed
    CmdContext.TransitionResource(BindingTable.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    CmdContext.UpdateBuffer(BindingTable.Get(), 0, TableEntrySize, &RayGenEntry);
    CmdContext.UpdateBuffer(BindingTable.Get(), TableEntrySize, TableEntrySize, &MissEntry);
    CmdContext.UpdateBuffer(BindingTable.Get(), TableEntrySize * 2, NumHitGroupResources * TableEntrySize, HitGroupEntries);
    CmdContext.TransitionResource(BindingTable.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    ShaderBindingTableBuilder.Reset();
    BindingTableHeaps[0] = ResourceHeap->GetNativeHeap();
    BindingTableHeaps[1] = SamplerHeap->GetNativeHeap();

    BindingTableStride = sizeof(SD3D12ShaderBindingTableEntry);
    NumHitGroups = NumHitGroupResources;

    return true;
}

void CD3D12RHIRayTracingScene::SetName(const String& InName)
{
    CRHIObject::SetName(InName);
    ResultBuffer->SetName(InName);

    if (ScratchBuffer)
    {
        ScratchBuffer->SetName(InName + " Scratch");
    }
    if (InstanceBuffer)
    {
        InstanceBuffer->SetName(InName + " Instance");
    }
    if (BindingTable)
    {
        BindingTable->SetName(InName + " BindingTable");
    }
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CD3D12RHIRayTracingScene::GetHitGroupTable() const
{
    Assert(BindingTable != nullptr);
    Assert(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    uint64 AddressOffset      = BindingTableStride * 2;
    uint64 SizeInBytes        = (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + AddressOffset, SizeInBytes, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE CD3D12RHIRayTracingScene::GetRayGenShaderRecord() const
{
    Assert(BindingTable != nullptr);
    Assert(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    return { BindingTableAdress, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CD3D12RHIRayTracingScene::GetMissShaderTable() const
{
    Assert(BindingTable != nullptr);
    Assert(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    uint64 AddressOffset      = BindingTableStride;
    return { BindingTableAdress + AddressOffset, BindingTableStride, BindingTableStride };
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ShaderBindingTableBuilder 

CD3D12ShaderBindingTableBuilder::CD3D12ShaderBindingTableBuilder(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
{
    Reset();
}

void CD3D12ShaderBindingTableBuilder::PopulateEntry(
    CD3D12RHIRayTracingPipelineState* PipelineState,
    CD3D12RootSignature* RootSignature,
    CD3D12OnlineDescriptorHeap* ResourceHeap,
    CD3D12OnlineDescriptorHeap* SamplerHeap,
    SD3D12ShaderBindingTableEntry& OutShaderBindingEntry,
    const SRayTracingShaderResources& Resources)
{
    Assert(PipelineState != nullptr);
    Assert(RootSignature != nullptr);
    Assert(ResourceHeap != nullptr);
    Assert(SamplerHeap != nullptr);

    CMemory::Memcpy(OutShaderBindingEntry.ShaderIdentifier, PipelineState->GetShaderIdentifer(Resources.Identifier), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    if (!Resources.ConstantBuffers.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_CBV);
        Assert(RootIndex < 4);

        uint32 NumDescriptors = Resources.ConstantBuffers.Size();
        uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex]       = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (CRHIConstantBuffer* CBuffer : Resources.ConstantBuffers)
        {
            CD3D12RHIConstantBuffer* DxConstantBuffer = static_cast<CD3D12RHIConstantBuffer*>(CBuffer);
            ResourceHandles[CPUResourceIndex++] = DxConstantBuffer->GetView().GetOfflineHandle();
        }
    }
    if (!Resources.ShaderResourceViews.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_SRV);
        Assert(RootIndex < 4);

        uint32 NumDescriptors = Resources.ShaderResourceViews.Size();
        uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex]       = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (CRHIShaderResourceView* ShaderResourceView : Resources.ShaderResourceViews)
        {
            CD3D12RHIShaderResourceView* DxShaderResourceView = static_cast<CD3D12RHIShaderResourceView*>(ShaderResourceView);
            ResourceHandles[CPUResourceIndex++] = DxShaderResourceView->GetOfflineHandle();
        }
    }
    if (!Resources.UnorderedAccessViews.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_UAV);
        Assert(RootIndex < 4);

        uint32 NumDescriptors = Resources.UnorderedAccessViews.Size();
        uint32 Handle = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex] = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (CRHIUnorderedAccessView* UnorderedAccessView : Resources.UnorderedAccessViews)
        {
            CD3D12RHIUnorderedAccessView* DxUnorderedAccessView = static_cast<CD3D12RHIUnorderedAccessView*>(UnorderedAccessView);
            ResourceHandles[CPUResourceIndex++] = DxUnorderedAccessView->GetOfflineHandle();
        }
    }
    if (!Resources.SamplerStates.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_Sampler);
        Assert(RootIndex < 4);

        uint32 NumDescriptors = Resources.SamplerStates.Size();
        uint32 Handle = SamplerHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = SamplerHeap->GetGPUDescriptorHandleAt(Handle);

        GPUSamplerHandles[GPUSamplerIndex] = SamplerHeap->GetCPUDescriptorHandleAt(Handle);
        GPUSamplerHandleSizes[GPUSamplerIndex++] = NumDescriptors;

        for (CRHISamplerState* Sampler : Resources.SamplerStates)
        {
            CD3D12RHISamplerState* DxSampler = static_cast<CD3D12RHISamplerState*>(Sampler);
            SamplerHandles[CPUSamplerIndex++] = DxSampler->GetOfflineHandle();
        }
    }
}

void CD3D12ShaderBindingTableBuilder::CopyDescriptors()
{
    GetDevice()->CopyDescriptors(
        GPUResourceIndex, GPUResourceHandles, GPUResourceHandleSizes,
        CPUResourceIndex, ResourceHandles, CPUHandleSizes,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    GetDevice()->CopyDescriptors(
        GPUSamplerIndex, GPUSamplerHandles, GPUSamplerHandleSizes,
        CPUSamplerIndex, SamplerHandles, CPUHandleSizes,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void CD3D12ShaderBindingTableBuilder::Reset()
{
    for (uint32 i = 0; i < ArrayCount(CPUHandleSizes); i++)
    {
        CPUHandleSizes[i] = 1;
    }

    CPUResourceIndex = 0;
    CPUSamplerIndex  = 0;
    GPUResourceIndex = 0;
    GPUSamplerIndex  = 0;
}
