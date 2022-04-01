#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "RHIInstanceD3D12.h"
#include "D3D12RayTracing.h"

#include "RHI/RHIModule.h"

#include "Engine/Assets/MeshFactory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingGeometry

CD3D12RayTracingGeometry::CD3D12RayTracingGeometry(CD3D12Device* InDevice, uint32 InFlags)
    : CRHIRayTracingGeometry(InFlags)
    , CD3D12DeviceObject(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
{
}

bool CD3D12RayTracingGeometry::Build(CD3D12CommandContext& CmdContext, bool bUpdate)
{
    Check(VertexBuffer != nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc;
    CMemory::Memzero(&GeometryDesc);

    GeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress  = VertexBuffer->GetResource()->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes = VertexBuffer->GetStride();
    GeometryDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
    GeometryDesc.Triangles.VertexCount                = VertexCount;
    GeometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    if (IndexBuffer)
    {
        GeometryDesc.Triangles.IndexFormat = (IndexFormat == ERHIIndexFormat::uint32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        GeometryDesc.Triangles.IndexBuffer = IndexBuffer->GetResource()->GetGPUVirtualAddress();
        GeometryDesc.Triangles.IndexCount  = IndexCount;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    CMemory::Memzero(&Inputs);

    Inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs       = 1;
    Inputs.pGeometryDescs = &GeometryDesc;
    Inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    Inputs.Flags          = ConvertAccelerationStructureBuildFlags(GetFlags());
    if (bUpdate)
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

        CD3D12ResourceRef Buffer = CD3D12Resource::CreateResource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr);
        if (!Buffer)
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

        CD3D12ResourceRef Buffer = CD3D12Resource::CreateResource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, nullptr);
        if (!Buffer)
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
// CD3D12RayTracingScene

CD3D12RayTracingScene::CD3D12RayTracingScene(CD3D12Device* InDevice, uint32 InFlags)
    : CRHIRayTracingScene(InFlags)
    , CD3D12DeviceObject(InDevice)
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

bool CD3D12RayTracingScene::Build(CD3D12CommandContext& CmdContext, const SRHIRayTracingGeometryInstance* InInstances, uint32 NumInstances, bool bUpdate)
{
    Check(InInstances != nullptr && NumInstances != 0);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    CMemory::Memzero(&Inputs);

    Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs    = NumInstances;
    Inputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    Inputs.Flags       = ConvertAccelerationStructureBuildFlags(GetFlags());
    if (bUpdate)
    {
        Check(GetFlags() & RayTracingStructureBuildFlag_AllowUpdate);
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

        CD3D12ResourceRef Buffer = CD3D12Resource::CreateResource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr);
        if (!Buffer)
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

        View = dbg_new CD3D12ShaderResourceView(GetDevice(), GD3D12RHIInstance->GetResourceOfflineDescriptorHeap());
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

        CD3D12ResourceRef Buffer = CD3D12Resource::CreateResource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, nullptr);
        if (!Buffer)
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
        CD3D12RayTracingGeometry* DxGeometry = static_cast<CD3D12RayTracingGeometry*>(InInstances[i].Instance.Get());
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

        CD3D12ResourceRef Buffer = CD3D12Resource::CreateResource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, nullptr);
        if (!Buffer)
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
    if (bUpdate)
    {
        Check(GetFlags() & RayTracingStructureBuildFlag_AllowUpdate);
        AccelerationStructureDesc.SourceAccelerationStructureData = ResultBuffer->GetGPUVirtualAddress();
    }

    CmdContext.FlushResourceBarriers();

    CD3D12CommandList& CmdList = CmdContext.GetCommandList();
    CmdList.BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    CmdContext.UnorderedAccessBarrier(ResultBuffer.Get());

    // Copy the instances
    Instances = TArray<SRHIRayTracingGeometryInstance>(InInstances, NumInstances);
    return true;
}

bool CD3D12RayTracingScene::BuildBindingTable(
    CD3D12CommandContext& CmdContext,
    CD3D12RayTracingPipelineState* PipelineState,
    CD3D12OnlineDescriptorHeap* ResourceHeap,
    CD3D12OnlineDescriptorHeap* SamplerHeap,
    const SRayTracingShaderResources* RayGenLocalResources,
    const SRayTracingShaderResources* MissLocalResources,
    const SRayTracingShaderResources* HitGroupResources,
    uint32 NumHitGroupResources)
{
    Check(ResourceHeap != nullptr);
    Check(SamplerHeap != nullptr);
    Check(PipelineState != nullptr);
    Check(RayGenLocalResources != nullptr);

    SD3D12ShaderBindingTableEntry RayGenEntry;
    ShaderBindingTableBuilder.PopulateEntry(
        PipelineState,
        PipelineState->GetRayGenLocalRootSignature(),
        ResourceHeap,
        SamplerHeap,
        RayGenEntry,
        *RayGenLocalResources);

    Check(MissLocalResources != nullptr);

    SD3D12ShaderBindingTableEntry MissEntry;
    ShaderBindingTableBuilder.PopulateEntry(
        PipelineState,
        PipelineState->GetMissLocalRootSignature(),
        ResourceHeap,
        SamplerHeap,
        MissEntry,
        *MissLocalResources);

    Check(HitGroupResources != nullptr);
    Check(NumHitGroupResources <= D3D12_MAX_HIT_GROUPS);

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
    const uint32 TableEntrySize   = sizeof(SD3D12ShaderBindingTableEntry);
    const uint64 BindingTableSize = TableEntrySize + TableEntrySize + (TableEntrySize * NumHitGroupResources);

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

        CD3D12ResourceRef Buffer = CD3D12Resource::CreateResource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, nullptr);
        if (!Buffer)
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
    BindingTableHeaps[0] = ResourceHeap->GetD3D12Heap();
    BindingTableHeaps[1] = SamplerHeap->GetD3D12Heap();

    BindingTableStride = sizeof(SD3D12ShaderBindingTableEntry);
    NumHitGroups = NumHitGroupResources;

    return true;
}

void CD3D12RayTracingScene::SetName(const String& InName)
{
    CRHIResource::SetName(InName);
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

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CD3D12RayTracingScene::GetHitGroupTable() const
{
    Check(BindingTable != nullptr);
    Check(BindingTableStride != 0);

    const uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    const uint64 AddressOffset      = BindingTableStride * 2;
    const uint64 SizeInBytes        = (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + AddressOffset, SizeInBytes, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE CD3D12RayTracingScene::GetRayGenShaderRecord() const
{
    Check(BindingTable != nullptr);
    Check(BindingTableStride != 0);

    const uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    return { BindingTableAdress, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CD3D12RayTracingScene::GetMissShaderTable() const
{
    Check(BindingTable != nullptr);
    Check(BindingTableStride != 0);

    const uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    const uint64 AddressOffset      = BindingTableStride;
    return { BindingTableAdress + AddressOffset, BindingTableStride, BindingTableStride };
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ShaderBindingTableBuilder 

CD3D12ShaderBindingTableBuilder::CD3D12ShaderBindingTableBuilder(CD3D12Device* InDevice)
    : CD3D12DeviceObject(InDevice)
{
    Reset();
}

void CD3D12ShaderBindingTableBuilder::PopulateEntry(
    CD3D12RayTracingPipelineState* PipelineState,
    CD3D12RootSignature* RootSignature,
    CD3D12OnlineDescriptorHeap* ResourceHeap,
    CD3D12OnlineDescriptorHeap* SamplerHeap,
    SD3D12ShaderBindingTableEntry& OutShaderBindingEntry,
    const SRayTracingShaderResources& Resources)
{
    Check(PipelineState != nullptr);
    Check(RootSignature != nullptr);
    Check(ResourceHeap  != nullptr);
    Check(SamplerHeap   != nullptr);

    CMemory::Memcpy(OutShaderBindingEntry.ShaderIdentifier, PipelineState->GetShaderIdentifer(Resources.Identifier), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    if (!Resources.ConstantBuffers.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_CBV);
        Check(RootIndex < 4);

        const uint32 NumDescriptors = Resources.ConstantBuffers.Size();
        const uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex] = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (CRHIBuffer* ConstantBuffer : Resources.ConstantBuffers)
        {
            CD3D12ConstantBufferView* ConstantBufferView = static_cast<CD3D12Buffer*>(ConstantBuffer)->GetConstantBufferView();
            ResourceHandles[CPUResourceIndex++] = ConstantBufferView->GetOfflineHandle();
        }
    }

    if (!Resources.ShaderResourceViews.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_SRV);
        Check(RootIndex < 4);

        const uint32 NumDescriptors = Resources.ShaderResourceViews.Size();
        const uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex] = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (CRHIShaderResourceView* ShaderResourceView : Resources.ShaderResourceViews)
        {
            CD3D12ShaderResourceView* DxShaderResourceView = static_cast<CD3D12ShaderResourceView*>(ShaderResourceView);
            ResourceHandles[CPUResourceIndex++] = DxShaderResourceView->GetOfflineHandle();
        }
    }

    if (!Resources.UnorderedAccessViews.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_UAV);
        Check(RootIndex < 4);

        const uint32 NumDescriptors = Resources.UnorderedAccessViews.Size();
        const uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex] = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (CRHIUnorderedAccessView* UnorderedAccessView : Resources.UnorderedAccessViews)
        {
            CD3D12UnorderedAccessView* DxUnorderedAccessView = static_cast<CD3D12UnorderedAccessView*>(UnorderedAccessView);
            ResourceHandles[CPUResourceIndex++] = DxUnorderedAccessView->GetOfflineHandle();
        }
    }

    if (!Resources.SamplerStates.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_Sampler);
        Check(RootIndex < 4);

        const uint32 NumDescriptors = Resources.SamplerStates.Size();
        const uint32 Handle         = SamplerHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = SamplerHeap->GetGPUDescriptorHandleAt(Handle);

        GPUSamplerHandles[GPUSamplerIndex] = SamplerHeap->GetCPUDescriptorHandleAt(Handle);
        GPUSamplerHandleSizes[GPUSamplerIndex++] = NumDescriptors;

        for (CRHISamplerState* Sampler : Resources.SamplerStates)
        {
            CD3D12SamplerState* DxSampler = static_cast<CD3D12SamplerState*>(Sampler);
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
