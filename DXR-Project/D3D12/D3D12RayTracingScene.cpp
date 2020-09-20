#include "D3D12RayTracingScene.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12RayTracingPipelineState.h"

#include "Rendering/MeshFactory.h"

/*
* D3D12RayTracingGeometry
*/

D3D12RayTracingGeometry::D3D12RayTracingGeometry(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, VertexBuffer(nullptr)
	, IndexBuffer(nullptr)
{
}

D3D12RayTracingGeometry::~D3D12RayTracingGeometry()
{
	SAFEDELETE(ScratchBuffer);
	SAFEDELETE(ResultBuffer);
}

bool D3D12RayTracingGeometry::BuildAccelerationStructure(D3D12CommandList* CommandList, TSharedPtr<D3D12Buffer>& InVertexBuffer, Uint32 InVertexCount, TSharedPtr<D3D12Buffer>& InIndexBuffer, Uint32 InIndexCount)
{
	if (!IsDirty)
	{
		return true;
	}

	D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc = {};
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
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
	Inputs.DescsLayout		= D3D12_ELEMENTS_LAYOUT_ARRAY;
	Inputs.Flags			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	Inputs.NumDescs			= 1;
	Inputs.pGeometryDescs	= &GeometryDesc;
	Inputs.Type				= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = { };
	Device->GetDXRDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
	BufferProperties BufferProps = { };
	BufferProps.SizeInBytes	= Info.ScratchDataSizeInBytes;
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	BufferProps.InitalState	= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;

	ScratchBuffer = new D3D12Buffer(Device);
	if (!ScratchBuffer->Initialize(BufferProps))
	{
		return false;
	}

	BufferProps.SizeInBytes = Info.ResultDataMaxSizeInBytes;
	BufferProps.InitalState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	ResultBuffer = new D3D12Buffer(Device);
	if (!ResultBuffer->Initialize(BufferProps))
	{
		return false;
	}

	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
	AccelerationStructureDesc.Inputs							= Inputs;
	AccelerationStructureDesc.DestAccelerationStructureData		= ResultBuffer->GetGPUVirtualAddress();
	AccelerationStructureDesc.ScratchAccelerationStructureData	= ScratchBuffer->GetGPUVirtualAddress();

	CommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	CommandList->UnorderedAccessBarrier(ResultBuffer);

	// Keep instance of buffers
	VertexBuffer	= InVertexBuffer;
	IndexBuffer		= InIndexBuffer;
	VertexCount		= InVertexCount;
	IndexCount		= InIndexCount;
	
	IsDirty = false;
	return true;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RayTracingGeometry::GetGPUVirtualAddress() const
{
	return ResultBuffer->GetGPUVirtualAddress();
}

void D3D12RayTracingGeometry::SetDebugName(const std::string& Name)
{
	ResultBuffer->SetDebugName(Name);
}

/*
* RayTracingScene
*/

D3D12RayTracingScene::D3D12RayTracingScene(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Instances()
{
}

D3D12RayTracingScene::~D3D12RayTracingScene()
{
	SAFEDELETE(ScratchBuffer);
	SAFEDELETE(ResultBuffer);
	SAFEDELETE(InstanceBuffer);
	SAFEDELETE(BindingTable);
}

bool D3D12RayTracingScene::Initialize(D3D12RayTracingPipelineState* PipelineState, TArray<BindingTableEntry>& InBindingTableEntries, Uint32 InNumHitGroups)
{
	using namespace Microsoft::WRL;

	// Create Shader Binding Table
	ComPtr<ID3D12StateObject>			StateObject = PipelineState->GetStateObject();
	ComPtr<ID3D12StateObjectProperties> StateProperties;
	
	HRESULT hResult = StateObject.As<ID3D12StateObjectProperties>(&StateProperties);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12RayTracingScene]: FAILED to retrive PipelineState Properties");
		return false;
	}
	else
	{
		LOG_INFO("[D3D12RayTracingScene]: Retrived PipelineState Properties");
	}

	// Struct for each entry in shaderbinding table
	struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) TableEntry
	{
		Byte ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
		D3D12_GPU_DESCRIPTOR_HANDLE	DescriptorTable;
	};

	const Uint32 StrideInBytes	= sizeof(TableEntry);
	const Uint32 SizeInBytes	= StrideInBytes * static_cast<Uint32>(InBindingTableEntries.Size());
	BindingTableStride = StrideInBytes;

	BufferProperties BufferProps = { };
	BufferProps.SizeInBytes = SizeInBytes;
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.InitalState = D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

	BindingTable = new D3D12Buffer(Device);
	if (!BindingTable->Initialize(BufferProps))
	{
		LOG_ERROR("[D3D12RayTracingScene]: FAILED to create BindingTable\n");
		return false;
	}

	// Map the buffer
	Byte* Data = reinterpret_cast<Byte*>(BindingTable->Map());
	for (BindingTableEntry& Entry : InBindingTableEntries)
	{
		TableEntry TableData;
		if (Entry.DescriptorTable)
		{
			TableData.DescriptorTable = Entry.DescriptorTable->GetGPUTableStartHandle();
		}
		else
		{
			TableData.DescriptorTable = { 0 };
		}

		std::wstring WideShaderExportName = ConvertToWide(Entry.ShaderExportName);
		memcpy(TableData.ShaderIdentifier, StateProperties->GetShaderIdentifier(WideShaderExportName.c_str()), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		memcpy(Data, &TableData, StrideInBytes);
		Data += StrideInBytes;
	}
	BindingTable->Unmap();

	NumHitGroups		= InNumHitGroups;
	BindingTableEntries	= InBindingTableEntries;
	return true;
}

bool D3D12RayTracingScene::BuildAccelerationStructure(D3D12CommandList* CommandList, TArray<D3D12RayTracingGeometryInstance>& InInstances)
{
	const Uint32 InstanceCount = static_cast<Uint32>(InInstances.Size());

	// First get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
	Inputs.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY;
	Inputs.Flags		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	Inputs.NumDescs		= InstanceCount;
	Inputs.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info;
	Device->GetDXRDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

	// Create the buffers
	BufferProperties BufferProps = { };
	BufferProps.SizeInBytes	= Info.ScratchDataSizeInBytes;
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	BufferProps.InitalState	= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;

	ScratchBuffer = new D3D12Buffer(Device);
	if (!ScratchBuffer->Initialize(BufferProps))
	{
		return false;
	}

	BufferProps.SizeInBytes = Info.ResultDataMaxSizeInBytes;
	BufferProps.InitalState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	ResultBuffer = new D3D12Buffer(Device);
	if (!ResultBuffer->Initialize(BufferProps))
	{
		return false;
	}

	BufferProps.SizeInBytes	= sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * InstanceCount;
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

	InstanceBuffer = new D3D12Buffer(Device);
	if(!InstanceBuffer->Initialize(BufferProps))
	{
		return false;
	}

	// Map and set each instance matrix
	D3D12_RAYTRACING_INSTANCE_DESC* InstanceDesc = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(InstanceBuffer->Map());
	for (Uint32 i = 0; i < InstanceCount; i++)
	{
		InstanceDesc->InstanceID							= InInstances[i].InstanceID;
		InstanceDesc->InstanceContributionToHitGroupIndex	= InInstances[i].HitGroupIndex;
		InstanceDesc->Flags									= D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		D3D12RayTracingGeometryInstance& Instance = InInstances[i];
		InstanceDesc->AccelerationStructure		= Instance.Geometry->GetGPUVirtualAddress();
		InstanceDesc->InstanceMask				= 0xFF;

		memcpy(InstanceDesc->Transform, &Instance.Transform, sizeof(InstanceDesc->Transform));

		InstanceDesc++;
	}

	// Unmap
	InstanceBuffer->Unmap();

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
	AccelerationStructureDesc.Inputs							= Inputs;
	AccelerationStructureDesc.Inputs.InstanceDescs				= InstanceBuffer->GetGPUVirtualAddress();
	AccelerationStructureDesc.DestAccelerationStructureData		= ResultBuffer->GetGPUVirtualAddress();
	AccelerationStructureDesc.ScratchAccelerationStructureData	= ScratchBuffer->GetGPUVirtualAddress();
	CommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

	// UAV barrier needed before using the acceleration structures in a raytracing operation
	CommandList->UnorderedAccessBarrier(ResultBuffer);

	// Copy the instances
	Instances = InInstances;

	// Create descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.ViewDimension								= D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	SrvDesc.Shader4ComponentMapping						= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.RaytracingAccelerationStructure.Location	= ResultBuffer->GetGPUVirtualAddress();

	View = MakeShared<D3D12ShaderResourceView>(Device, nullptr, &SrvDesc);

	IsDirty = false;
	return true;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RayTracingScene::GetGPUVirtualAddress() const
{
	return ResultBuffer->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE D3D12RayTracingScene::GetRayGenerationShaderRecord() const
{
	const Uint64 BindingTableAdress = BindingTable->GetGPUVirtualAddress();
	return { BindingTableAdress, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingScene::GetHitGroupTable() const
{
	const Uint64 BindingTableAdress	= BindingTable->GetGPUVirtualAddress();
	const Uint64 SizeInBytes		= (BindingTableStride * NumHitGroups);
	return { BindingTableAdress + BindingTableStride, SizeInBytes, BindingTableStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingScene::GetMissShaderTable() const
{
	const Uint64 BindingTableAdress		= BindingTable->GetGPUVirtualAddress();
	const Uint64 HitGroupSizeInBytes	= (BindingTableStride * NumHitGroups);
	return { BindingTableAdress + BindingTableStride + HitGroupSizeInBytes, BindingTableStride, BindingTableStride };
}

void D3D12RayTracingScene::SetDebugName(const std::string& Name)
{
	ResultBuffer->SetDebugName(Name);
}
