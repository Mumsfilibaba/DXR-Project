#include "Rendering/MeshFactory.h"

#include "RenderLayer/RenderLayer.h"

#include "D3D12RenderLayer.h"
#include "D3D12RayTracingScene.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12RayTracingPipelineState.h"

D3D12RayTracingGeometry::D3D12RayTracingGeometry(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
{
}

D3D12RayTracingGeometry::~D3D12RayTracingGeometry()
{
    SAFERELEASE(ScratchBuffer);
    SAFERELEASE(ResultBuffer);
}

bool D3D12RayTracingGeometry::BuildAccelerationStructure(
    D3D12CommandListHandle* CommandList, 
    TSharedRef<D3D12VertexBuffer>& InVertexBuffer,
    UInt32 InVertexCount, 
    TSharedRef<D3D12IndexBuffer>& InIndexBuffer,
    UInt32 InIndexCount)
{
    if (!IsDirty)
    {
        return true;
    }

    D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc;
    Memory::Memzero(&GeometryDesc, sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));

    GeometryDesc.Type									= D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeometryDesc.Triangles.VertexBuffer.StartAddress	= InVertexBuffer->GetGPUVirtualAddress();
    GeometryDesc.Triangles.VertexBuffer.StrideInBytes	= sizeof(Vertex);
    GeometryDesc.Triangles.VertexFormat					= DXGI_FORMAT_R32G32B32_FLOAT;
    GeometryDesc.Triangles.VertexCount					= InVertexCount;
    GeometryDesc.Triangles.IndexFormat					= DXGI_FORMAT_R32_UINT;
    GeometryDesc.Triangles.IndexBuffer					= InIndexBuffer->GetGPUVirtualAddress();
    GeometryDesc.Triangles.IndexCount					= InIndexCount;
    GeometryDesc.Flags									= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get the size requirements for the scratch and AS buffers
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    Memory::Memzero(&Inputs, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS));

    Inputs.DescsLayout		= D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.Flags			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    Inputs.NumDescs			= 1;
    Inputs.pGeometryDescs	= &GeometryDesc;
    Inputs.Type				= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info;
    Memory::Memzero(&Info, sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO));
    Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

    ScratchBuffer = static_cast<D3D12StructuredBuffer*>(RenderLayer::CreateStructuredBuffer(
        nullptr, 
        UInt32(Info.ScratchDataSizeInBytes), 
        1,
        BufferUsage_UAV | BufferUsage_Default));
    if (!ScratchBuffer)
    {
        return false;
    }

    ResultBuffer = static_cast<D3D12StructuredBuffer*>(RenderLayer::CreateStructuredBuffer(
        nullptr, 
        UInt32(Info.ResultDataMaxSizeInBytes), 
        1,
        BufferUsage_UAV | BufferUsage_Default));
    if (!ResultBuffer)
    {
        return false;
    }

    // Create the bottom-level AS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc;
    Memory::Memzero(&AccelerationStructureDesc, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC));

    AccelerationStructureDesc.Inputs							= Inputs;
    AccelerationStructureDesc.DestAccelerationStructureData		= ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData	= ScratchBuffer->GetGPUVirtualAddress();

    CommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    // We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
    //CommandList->UnorderedAccessBarrier(ResultBuffer);

    // Keep instance of buffers
    VertexBuffer	= InVertexBuffer;
    IndexBuffer		= InIndexBuffer;
    VertexCount		= InVertexCount;
    IndexCount		= InIndexCount;
    
    // Write descriptors
    //DescriptorTable->SetShaderResourceView(VertexBuffer->GetShaderResourceView(0).Get(), 0);
    //DescriptorTable->SetShaderResourceView(IndexBuffer->GetShaderResourceView(0).Get(), 1);
    //DescriptorTable->CopyDescriptors();

    IsDirty = false;
    return true;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RayTracingGeometry::GetGPUVirtualAddress() const
{
    return ResultBuffer->GetGPUVirtualAddress();
}

void D3D12RayTracingGeometry::SetName(const std::string& Name)
{
    ResultBuffer->SetName(Name);
}

/*
* RayTracingScene
*/

D3D12RayTracingScene::D3D12RayTracingScene(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
    , ResultBuffer(nullptr)
    , ScratchBuffer(nullptr)
    , InstanceBuffer(nullptr)
    , BindingTable(nullptr)
    , BindingTableStride(0)
    , NumHitGroups(0)
    , View(nullptr)
    , Instances()
    , BindingTableEntries()
    , PipelineStateProperties(nullptr)
    , IsDirty(true)
{
}

D3D12RayTracingScene::~D3D12RayTracingScene()
{
    SAFEDELETE(ScratchBuffer);
    SAFEDELETE(ResultBuffer);
    SAFEDELETE(InstanceBuffer);
    SAFEDELETE(BindingTable);
}

bool D3D12RayTracingScene::Initialize(D3D12RayTracingPipelineState* PipelineState)
{
    using namespace Microsoft::WRL;

    // Create Shader Binding Table
    ComPtr<ID3D12StateObject> StateObject = PipelineState->GetStateObject();
    HRESULT hResult = StateObject.As<ID3D12StateObjectProperties>(&PipelineStateProperties);
    if (FAILED(hResult))
    {
        LOG_ERROR("[D3D12RayTracingScene]: FAILED to retrive PipelineState Properties");
        return false;
    }
    else
    {
        LOG_INFO("[D3D12RayTracingScene]: Retrived PipelineState Properties");
    }

    return true;
}

bool D3D12RayTracingScene::BuildAccelerationStructure(
    D3D12CommandListHandle* CommandList, 
    TArray<D3D12RayTracingGeometryInstance>& InInstances,
    TArray<BindingTableEntry>& InBindingTableEntries,
    UInt32 InNumHitGroups)
{
    if (!IsDirty)
    {
        return true;
    }

    // Struct for each entry in shaderbinding table
    struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) TableEntry
    {
        Byte ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
        D3D12_GPU_DESCRIPTOR_HANDLE	DescriptorTable0;
        D3D12_GPU_DESCRIPTOR_HANDLE	DescriptorTable1;
    };

    const UInt32 StrideInBytes	= sizeof(TableEntry);
    const UInt32 SizeInBytes	= StrideInBytes * static_cast<UInt32>(InBindingTableEntries.Size());
    BindingTableStride = StrideInBytes;

    BindingTable = static_cast<D3D12StructuredBuffer*>(
        RenderLayer::CreateStructuredBuffer(nullptr, SizeInBytes, 1, BufferUsage_UAV | BufferUsage_Dynamic));
    if (!BindingTable)
    {
        LOG_ERROR("[D3D12RayTracingScene]: FAILED to create BindingTable\n");
        return false;
    }

    // Map the buffer
    Byte* Data = reinterpret_cast<Byte*>(BindingTable->Map(nullptr));
    for (BindingTableEntry& Entry : InBindingTableEntries)
    {
        TableEntry TableData;
        //if (Entry.DescriptorTable0)
        //{
        //	TableData.DescriptorTable0 = Entry.DescriptorTable0->GetGPUTableStartHandle();
        //}
        //else
        {
            TableData.DescriptorTable0 = { 0 };
        }

        //if (Entry.DescriptorTable1)
        //{
        //	TableData.DescriptorTable1 = Entry.DescriptorTable1->GetGPUTableStartHandle();
        //}
        //else
        {
            TableData.DescriptorTable1 = { 0 };
        }

        std::wstring WideShaderExportName = ConvertToWide(Entry.ShaderExportName);
        Memory::Memcpy(TableData.ShaderIdentifier, PipelineStateProperties->GetShaderIdentifier(WideShaderExportName.c_str()), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        Memory::Memcpy(Data, &TableData, StrideInBytes);
        Data += StrideInBytes;
    }
    BindingTable->Unmap(nullptr);

    NumHitGroups		= InNumHitGroups;
    BindingTableEntries	= InBindingTableEntries;

    // Init accelerationstructure
    const UInt32 InstanceCount = static_cast<UInt32>(InInstances.Size());

    // First get the size of the TLAS buffers and create them
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs;
    Memory::Memzero(&Inputs, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS));

    Inputs.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.Flags		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    Inputs.NumDescs		= InstanceCount;
    Inputs.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info;
    Memory::Memzero(&Info, sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO));
    Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

    // Create the buffers
    ScratchBuffer = static_cast<D3D12StructuredBuffer*>(RenderLayer::CreateStructuredBuffer(
        nullptr, 
        UInt32(Info.ScratchDataSizeInBytes), 
        1, 
        BufferUsage_UAV | BufferUsage_Default));
    if (!ScratchBuffer)
    {
        return false;
    }

    ResultBuffer = static_cast<D3D12StructuredBuffer*>(RenderLayer::CreateStructuredBuffer(
        nullptr, 
        UInt32(Info.ResultDataMaxSizeInBytes), 
        1, 
        BufferUsage_UAV | BufferUsage_Default));
    if (!ResultBuffer)
    {
        return false;
    }

    const UInt32 InstanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * InstanceCount;
    InstanceBuffer = static_cast<D3D12StructuredBuffer*>(
        RenderLayer::CreateStructuredBuffer(nullptr, InstanceBufferSize, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), BufferUsage_UAV | BufferUsage_Dynamic));
    if(!InstanceBuffer)
    {
        return false;
    }

    // Map and set each instance matrix
    D3D12_RAYTRACING_INSTANCE_DESC* InstanceDesc = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(InstanceBuffer->Map(nullptr));
    for (UInt32 i = 0; i < InstanceCount; i++)
    {
        InstanceDesc->InstanceID							= InInstances[i].InstanceID;
        InstanceDesc->InstanceContributionToHitGroupIndex	= InInstances[i].HitGroupIndex;
        InstanceDesc->Flags									= D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

        D3D12RayTracingGeometryInstance& Instance = InInstances[i];
        InstanceDesc->AccelerationStructure	= Instance.Geometry->GetGPUVirtualAddress();
        InstanceDesc->InstanceMask			= 0xFF;

        memcpy(InstanceDesc->Transform, &Instance.Transform, sizeof(InstanceDesc->Transform));

        InstanceDesc++;
    }

    // Unmap
    InstanceBuffer->Unmap(nullptr);

    // Create the TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc;
    Memory::Memzero(&AccelerationStructureDesc, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC));

    AccelerationStructureDesc.Inputs							= Inputs;
    AccelerationStructureDesc.Inputs.InstanceDescs				= InstanceBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.DestAccelerationStructureData		= ResultBuffer->GetGPUVirtualAddress();
    AccelerationStructureDesc.ScratchAccelerationStructureData	= ScratchBuffer->GetGPUVirtualAddress();
    CommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

    // UAV barrier needed before using the acceleration structures in a raytracing operation
    //CommandList->UnorderedAccessBarrier(ResultBuffer);

    // Copy the instances
    Instances = InInstances;

    // Create descriptor
    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
    Memory::Memzero(&SrvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

    SrvDesc.ViewDimension								= D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    SrvDesc.Shader4ComponentMapping						= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SrvDesc.RaytracingAccelerationStructure.Location	= ResultBuffer->GetGPUVirtualAddress();

    View = DBG_NEW D3D12ShaderResourceView(Device, nullptr, SrvDesc);

    IsDirty = false;
    return true;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RayTracingScene::GetGPUVirtualAddress() const
{
    return ResultBuffer->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE D3D12RayTracingScene::GetRayGenerationShaderRecord() const
{
    const UInt64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
    return { BindingTableAdress, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingScene::GetHitGroupTable() const
{
    const UInt64 BindingTableAdress	= BindingTable->GetGPUVirtualAddress();
    const UInt64 SizeInBytes		= (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + BindingTableStride, SizeInBytes, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingScene::GetMissShaderTable() const
{
    const UInt64 BindingTableAdress		= BindingTable->GetGPUVirtualAddress();
    const UInt64 HitGroupSizeInBytes	= (BindingTableStride * NumHitGroups);
    return { BindingTableAdress + BindingTableStride + HitGroupSizeInBytes, BindingTableStride, BindingTableStride };
}

void D3D12RayTracingScene::SetName(const std::string& Name)
{
    ResultBuffer->SetName(Name);
}
