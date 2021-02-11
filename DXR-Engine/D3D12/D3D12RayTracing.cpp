#include "RenderLayer/RenderLayer.h"

#include "Rendering/Resources/MeshFactory.h"

#include "D3D12RenderLayer.h"
#include "D3D12RayTracing.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"

D3D12RayTracingGeometry::D3D12RayTracingGeometry(D3D12Device* InDevice, UInt32 InFlags)
    : RayTracingGeometry(InFlags)
    , D3D12DeviceChild(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
{
}

Bool D3D12RayTracingGeometry::Build(D3D12CommandContext& CmdContext, Bool Update)
{
    Assert(VertexBuffer != nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc;
    Memory::Memzero(&GeometryDesc);

    GeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress  = VertexBuffer->GetResource()->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes = VertexBuffer->GetStride();
    GeometryDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
    GeometryDesc.Triangles.VertexCount                = VertexBuffer->GetNumVertices();
    GeometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    if (IndexBuffer)
    {
        EIndexFormat IndexFormat = IndexBuffer->GetFormat();
        GeometryDesc.Triangles.IndexFormat = IndexFormat == EIndexFormat::UInt32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        GeometryDesc.Triangles.IndexBuffer = IndexBuffer->GetResource()->GetGPUVirtualAddress();
        GeometryDesc.Triangles.IndexCount  = IndexBuffer->GetNumIndicies();
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    Memory::Memzero(&Inputs);

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
    Memory::Memzero(&PreBuildInfo);
    Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    UInt64 CurrentSize = ResultBuffer ? ResultBuffer->GetWidth() : 0;
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        D3D12_RESOURCE_DESC Desc;
        Memory::Memzero(&Desc);

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

        TRef<D3D12Resource> Buffer = DBG_NEW D3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr))
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            ResultBuffer = Buffer;
        }
    }

    UInt64 RequiredSize = Math::Max(PreBuildInfo.ScratchDataSizeInBytes, PreBuildInfo.UpdateScratchDataSizeInBytes);
    CurrentSize = ScratchBuffer ? ScratchBuffer->GetWidth() : 0;
    if (CurrentSize < RequiredSize)
    {
        D3D12_RESOURCE_DESC Desc;
        Memory::Memzero(&Desc);

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

        TRef<D3D12Resource> Buffer = DBG_NEW D3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            ScratchBuffer = Buffer;
        }
        
        CmdContext.TransitionResource(ScratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc;
    Memory::Memzero(&AccelerationStructureDesc);

    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();

    CmdContext.FlushResourceBarriers();

    D3D12CommandListHandle& CmdList = CmdContext.GetCommandList();
    CmdList.BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    CmdContext.UnorderedAccessBarrier(ResultBuffer.Get());

    return true;
}

D3D12RayTracingScene::D3D12RayTracingScene(D3D12Device* InDevice, UInt32 InFlags)
    : RayTracingScene(InFlags)
    , D3D12DeviceChild(InDevice)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
    , InstanceBuffer(nullptr)
    , BindingTable(nullptr)
    , BindingTableStride(0)
    , NumHitGroups(0)
    , View(nullptr)
    , Instances()
{
}

Bool D3D12RayTracingScene::Build(D3D12CommandContext& CmdContext, TArrayView<RayTracingGeometryInstance> InInstances, Bool Update)
{
    //// Struct for each entry in shaderbinding table
    struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) TableEntry
    {
        Byte ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
        D3D12_GPU_DESCRIPTOR_HANDLE	DescriptorTable0;
        D3D12_GPU_DESCRIPTOR_HANDLE	DescriptorTable1;
    };

    //const UInt32 StrideInBytes	= sizeof(TableEntry);
    //const UInt32 SizeInBytes	= StrideInBytes * static_cast<UInt32>(InBindingTableEntries.Size());
    //BindingTableStride = StrideInBytes;

    //BindingTable = static_cast<D3D12StructuredBuffer*>(
    //    RenderLayer::CreateStructuredBuffer(nullptr, SizeInBytes, 1, BufferUsage_UAV | BufferUsage_Dynamic));
    //if (!BindingTable)
    //{
    //    LOG_ERROR("[D3D12RayTracingScene]: FAILED to create BindingTable\n");
    //    return false;
    //}

    // Map the buffer
    //Byte* Data = reinterpret_cast<Byte*>(BindingTable->Map(nullptr));
    //for (BindingTableEntry& Entry : InBindingTableEntries)
    //{
    //    TableEntry TableData;
    //    //if (Entry.DescriptorTable0)
    //    //{
    //    //	TableData.DescriptorTable0 = Entry.DescriptorTable0->GetGPUTableStartHandle();
    //    //}
    //    //else
    //    {
    //        TableData.DescriptorTable0 = { 0 };
    //    }

    //    //if (Entry.DescriptorTable1)
    //    //{
    //    //	TableData.DescriptorTable1 = Entry.DescriptorTable1->GetGPUTableStartHandle();
    //    //}
    //    //else
    //    {
    //        TableData.DescriptorTable1 = { 0 };
    //    }

    //    std::wstring WideShaderExportName = ConvertToWide(Entry.ShaderExportName);
    //    Memory::Memcpy(TableData.ShaderIdentifier, PipelineStateProperties->GetShaderIdentifier(WideShaderExportName.c_str()), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    //    Memory::Memcpy(Data, &TableData, StrideInBytes);
    //    Data += StrideInBytes;
    //}
    //BindingTable->Unmap(nullptr);

    //NumHitGroups		= InNumHitGroups;
    //BindingTableEntries	= InBindingTableEntries;

    // Init accelerationstructure
    Assert(InInstances.IsEmpty() == false);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    Memory::Memzero(&Inputs);

    Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.NumDescs    = InInstances.Size();
    Inputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    Inputs.Flags       = ConvertAccelerationStructureBuildFlags(GetFlags());
    if (Update && GetFlags() & RayTracingStructureBuildFlag_AllowUpdate)
    {
        Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PreBuildInfo;
    Memory::Memzero(&PreBuildInfo);
    Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PreBuildInfo);

    UInt64 CurrentSize = ResultBuffer ? ResultBuffer->GetWidth() : 0;
    if (CurrentSize < PreBuildInfo.ResultDataMaxSizeInBytes)
    {
        D3D12_RESOURCE_DESC Desc;
        Memory::Memzero(&Desc);

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

        TRef<D3D12Resource> Buffer = DBG_NEW D3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr))
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            ResultBuffer = Buffer;
        }
    }

    UInt64 RequiredSize = Math::Max(PreBuildInfo.ScratchDataSizeInBytes, PreBuildInfo.UpdateScratchDataSizeInBytes);
    CurrentSize = ScratchBuffer ? ScratchBuffer->GetWidth() : 0;
    if (CurrentSize < RequiredSize)
    {
        D3D12_RESOURCE_DESC Desc;
        Memory::Memzero(&Desc);

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

        TRef<D3D12Resource> Buffer = DBG_NEW D3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            ScratchBuffer = Buffer;
        }

        CmdContext.TransitionResource(ScratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    // Map and set each instance matrix
    TArray<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs(InInstances.Size());
    for (UInt32 i = 0; i < InstanceDescs.Size(); i++)
    {
        D3D12RayTracingGeometry* DxGeometry = static_cast<D3D12RayTracingGeometry*>(InInstances[i].Instance.Get());
        InstanceDescs[i].AccelerationStructure = DxGeometry->GetGPUVirtualAddress();
        InstanceDescs[i].InstanceID            = i;
        InstanceDescs[i].Flags                 = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        InstanceDescs[i].InstanceMask          = 0xff;
        InstanceDescs[i].InstanceContributionToHitGroupIndex = i;

        Memory::Memcpy(&InstanceDescs[i].Transform, &InInstances[i].Transform, sizeof(XMFLOAT3X4));
    }

    CurrentSize = InstanceBuffer ? InstanceBuffer->GetWidth() : 0;
    if (CurrentSize < InstanceDescs.SizeInBytes())
    {
        D3D12_RESOURCE_DESC Desc;
        Memory::Memzero(&Desc);

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

        TRef<D3D12Resource> Buffer = DBG_NEW D3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
        if (!Buffer->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
        {
            Debug::DebugBreak();
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
    Memory::Memzero(&AccelerationStructureDesc);

    AccelerationStructureDesc.Inputs                           = Inputs;
    AccelerationStructureDesc.Inputs.InstanceDescs             = InstanceBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.DestAccelerationStructureData    = ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
    if (Update && GetFlags() & RayTracingStructureBuildFlag_AllowUpdate)
    {
        AccelerationStructureDesc.SourceAccelerationStructureData = ResultBuffer->GetGPUVirtualAddress();
    }

    CmdContext.FlushResourceBarriers();

    D3D12CommandListHandle& CmdList = CmdContext.GetCommandList();
    CmdList.BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    CmdContext.UnorderedAccessBarrier(ResultBuffer.Get());

    // Copy the instances
    Instances = TArray<RayTracingGeometryInstance>(InInstances.Begin(), InInstances.End());
    return true;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE D3D12RayTracingScene::GetRayGenerationShaderRecord() const
{
    const UInt64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    return { BindingTableAdress, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingScene::GetHitGroupTable() const
{
    const UInt64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    const UInt64 SizeInBytes        = (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + BindingTableStride, SizeInBytes, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingScene::GetMissShaderTable() const
{
    const UInt64 BindingTableAdress  = BindingTable->GetGPUVirtualAddress();
    const UInt64 HitGroupSizeInBytes = (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + BindingTableStride + HitGroupSizeInBytes, BindingTableStride, BindingTableStride };
}
