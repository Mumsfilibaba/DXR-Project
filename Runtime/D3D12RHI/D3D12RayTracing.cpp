#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CoreInterface.h"
#include "D3D12RayTracing.h"

#include "RHI/RHIModule.h"

#include "Engine/Assets/MeshFactory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingGeometry

CD3D12AccelerationStructure::CD3D12AccelerationStructure(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
{ }

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingGeometry

CD3D12RayTracingGeometry::CD3D12RayTracingGeometry(CD3D12Device* InDevice, const CRHIRayTracingGeometryInitializer& Initializer)
    : CRHIRayTracingGeometry(Initializer)
    , CD3D12AccelerationStructure(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
{ }

bool CD3D12RayTracingGeometry::Build(CD3D12CommandContext& CmdContext, CD3D12VertexBuffer* InVertexBuffer, CD3D12IndexBuffer* InIndexBuffer, bool bUpdate)
{
    Check(VertexBuffer != nullptr);

    VertexBuffer = MakeSharedRef<CD3D12VertexBuffer>(InVertexBuffer);
    IndexBuffer  = MakeSharedRef<CD3D12IndexBuffer>(InIndexBuffer);

    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc;
    CMemory::Memzero(&GeometryDesc);

    GeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress  = VertexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes = VertexBuffer->GetStride();
    GeometryDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
    GeometryDesc.Triangles.VertexCount                = VertexBuffer->GetNumVertices();
    GeometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    if (IndexBuffer)
    {
        EIndexFormat IndexFormat           = IndexBuffer->GetFormat();
        GeometryDesc.Triangles.IndexFormat = IndexFormat == EIndexFormat::uint32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        GeometryDesc.Triangles.IndexBuffer = IndexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
        GeometryDesc.Triangles.IndexCount  = IndexBuffer->GetNumIndicies();
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
// CD3D12RayTracingScene

CD3D12RayTracingScene::CD3D12RayTracingScene(CD3D12Device* InDevice, const CRHIRayTracingSceneInitializer& Initializer)
    : CRHIRayTracingScene(Initializer)
    , CD3D12AccelerationStructure(InDevice)
    , InstanceBuffer(nullptr)
    , BindingTable(nullptr)
    , BindingTableStride(0)
    , NumHitGroups(0)
    , View(nullptr)
    , Instances()
    , ShaderBindingTableBuilder(InDevice)
{ }

bool CD3D12RayTracingScene::Build(CD3D12CommandContext& CmdContext, const TArrayView<const CRHIRayTracingGeometryInstance>& InInstances, bool bUpdate)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    CMemory::Memzero(&Inputs);

    Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs    = InInstances.Size();
    Inputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    Inputs.Flags       = ConvertAccelerationStructureBuildFlags(GetFlags());
    
    if (bUpdate)
    {
        Check((GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None);
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

        View = dbg_new CD3D12ShaderResourceView(GetDevice(), GD3D12Instance->GetResourceOfflineDescriptorHeap(), this);
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

    TArray<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs(InInstances.Size());
    for (int32 Instance = 0; Instance < InstanceDescs.Size(); Instance++)
    {
        CD3D12RayTracingGeometry* D3D12Geometry = static_cast<CD3D12RayTracingGeometry*>(InInstances[Instance].Geometry);
        CMemory::Memcpy(&InstanceDescs[Instance].Transform, &InInstances[Instance].Transform, sizeof(CMatrix3x4));

        InstanceDescs[Instance].AccelerationStructure               = D3D12Geometry->GetGPUVirtualAddress();
        InstanceDescs[Instance].InstanceID                          = InInstances[Instance].InstanceIndex;
        InstanceDescs[Instance].Flags                               = ConvertRayTracingInstanceFlags(InInstances[Instance].Flags);
        InstanceDescs[Instance].InstanceMask                        = InInstances[Instance].Mask;
        InstanceDescs[Instance].InstanceContributionToHitGroupIndex = InInstances[Instance].HitGroupIndex;
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
    if (bUpdate)
    {
        Check((GetFlags() & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None);
        AccelerationStructureDesc.SourceAccelerationStructureData = ResultBuffer->GetGPUVirtualAddress();
    }

    CmdContext.FlushResourceBarriers();

    CD3D12CommandList& CmdList = CmdContext.GetCommandList();
    CmdList.BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    CmdContext.UnorderedAccessBarrier(ResultBuffer.Get());

    Instances.Reset(InInstances);
    return true;
}

bool CD3D12RayTracingScene::BuildBindingTable( CD3D12CommandContext& CmdContext
                                             , CD3D12RayTracingPipelineState* PipelineState
                                             , CD3D12OnlineDescriptorHeap* ResourceHeap
                                             , CD3D12OnlineDescriptorHeap* SamplerHeap
                                             , const SRayTracingShaderResources* RayGenLocalResources
                                             , const SRayTracingShaderResources* MissLocalResources
                                             , const SRayTracingShaderResources* HitGroupResources
                                             , uint32 NumHitGroupResources)
{
    Check(ResourceHeap         != nullptr);
    Check(SamplerHeap          != nullptr);
    Check(PipelineState        != nullptr);
    Check(RayGenLocalResources != nullptr);

    SD3D12ShaderBindingTableEntry RayGenEntry;
    ShaderBindingTableBuilder.PopulateEntry( PipelineState
                                           , PipelineState->GetRayGenLocalRootSignature()
                                           , ResourceHeap
                                           , SamplerHeap
                                           , RayGenEntry
                                           , *RayGenLocalResources);

    Check(MissLocalResources != nullptr);

    SD3D12ShaderBindingTableEntry MissEntry;
    ShaderBindingTableBuilder.PopulateEntry( PipelineState
                                           , PipelineState->GetMissLocalRootSignature()
                                           , ResourceHeap
                                           , SamplerHeap
                                           , MissEntry
                                           , *MissLocalResources);

    Check(HitGroupResources != nullptr);
    Check(NumHitGroupResources <= D3D12_MAX_HIT_GROUPS);

    SD3D12ShaderBindingTableEntry HitGroupEntries[D3D12_MAX_HIT_GROUPS];
    for (uint32 i = 0; i < NumHitGroupResources; i++)
    {
        ShaderBindingTableBuilder.PopulateEntry( PipelineState
                                               , PipelineState->GetHitLocalRootSignature()
                                               , ResourceHeap
                                               , SamplerHeap
                                               , HitGroupEntries[i]
                                               , HitGroupResources[i]);
    }

    ShaderBindingTableBuilder.CopyDescriptors();

    // TODO: More dynamic size of binding table
    uint32 TableEntrySize   = sizeof(SD3D12ShaderBindingTableEntry);
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

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CD3D12RayTracingScene::GetHitGroupTable() const
{
    Check(BindingTable != nullptr);
    Check(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    uint64 AddressOffset      = BindingTableStride * 2;
    uint64 SizeInBytes        = (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + AddressOffset, SizeInBytes, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE CD3D12RayTracingScene::GetRayGenShaderRecord() const
{
    Check(BindingTable != nullptr);
    Check(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    return { BindingTableAdress, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CD3D12RayTracingScene::GetMissShaderTable() const
{
    Check(BindingTable != nullptr);
    Check(BindingTableStride != 0);

    uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    uint64 AddressOffset      = BindingTableStride;
    return { BindingTableAdress + AddressOffset, BindingTableStride, BindingTableStride };
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ShaderBindingTableBuilder 

CD3D12ShaderBindingTableBuilder::CD3D12ShaderBindingTableBuilder(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
{
    Reset();
}

void CD3D12ShaderBindingTableBuilder::PopulateEntry( CD3D12RayTracingPipelineState* PipelineState
                                                   , CD3D12RootSignature* RootSignature
                                                   , CD3D12OnlineDescriptorHeap* ResourceHeap
                                                   , CD3D12OnlineDescriptorHeap* SamplerHeap
                                                   , SD3D12ShaderBindingTableEntry& OutShaderBindingEntry
                                                   , const SRayTracingShaderResources& Resources)
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

        uint32 NumDescriptors = Resources.ConstantBuffers.Size();
        uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex]       = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
        GPUResourceHandleSizes[GPUResourceIndex++] = NumDescriptors;

        for (CRHIConstantBuffer* CBuffer : Resources.ConstantBuffers)
        {
            CD3D12ConstantBuffer* DxConstantBuffer = static_cast<CD3D12ConstantBuffer*>(CBuffer);
            ResourceHandles[CPUResourceIndex++] = DxConstantBuffer->GetView().GetOfflineHandle();
        }
    }
    if (!Resources.ShaderResourceViews.IsEmpty())
    {
        uint32 RootIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, ResourceType_SRV);
        Check(RootIndex < 4);

        uint32 NumDescriptors = Resources.ShaderResourceViews.Size();
        uint32 Handle         = ResourceHeap->AllocateHandles(NumDescriptors);
        OutShaderBindingEntry.RootDescriptorTables[RootIndex] = ResourceHeap->GetGPUDescriptorHandleAt(Handle);

        GPUResourceHandles[GPUResourceIndex]       = ResourceHeap->GetCPUDescriptorHandleAt(Handle);
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

        uint32 NumDescriptors = Resources.UnorderedAccessViews.Size();
        uint32 Handle = ResourceHeap->AllocateHandles(NumDescriptors);
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

        uint32 NumDescriptors = Resources.SamplerStates.Size();
        uint32 Handle = SamplerHeap->AllocateHandles(NumDescriptors);
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
    GetDevice()->CopyDescriptors( GPUResourceIndex
                                , GPUResourceHandles
                                , GPUResourceHandleSizes
                                , CPUResourceIndex
                                , ResourceHandles
                                , CPUHandleSizes
                                , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    GetDevice()->CopyDescriptors( GPUSamplerIndex
                                , GPUSamplerHandles
                                , GPUSamplerHandleSizes
                                , CPUSamplerIndex
                                , SamplerHandles
                                , CPUHandleSizes
                                , D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
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
