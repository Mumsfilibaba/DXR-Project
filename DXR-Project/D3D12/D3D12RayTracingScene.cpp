#include "D3D12RayTracingScene.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "HeapProps.h"

#include "Rendering/MeshFactory.h"

/*
* D3D12RayTracingGeometry
*/

D3D12RayTracingGeometry::D3D12RayTracingGeometry(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12RayTracingGeometry::~D3D12RayTracingGeometry()
{
	SAFEDELETE(ScratchBuffer);
	SAFEDELETE(ResultBuffer);
}

bool D3D12RayTracingGeometry::Initialize(D3D12CommandList* InCommandList, std::shared_ptr<D3D12Buffer> InVertexBuffer, Uint32 VertexCount, std::shared_ptr<D3D12Buffer> InIndexBuffer, Uint32 IndexCount)
{
	D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc = {};
	GeometryDesc.Type									= D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	GeometryDesc.Triangles.VertexBuffer.StartAddress	= InVertexBuffer->GetVirtualAddress();
	GeometryDesc.Triangles.VertexBuffer.StrideInBytes	= sizeof(Vertex);
	GeometryDesc.Triangles.VertexFormat					= DXGI_FORMAT_R32G32B32_FLOAT;
	GeometryDesc.Triangles.VertexCount					= VertexCount;
	GeometryDesc.Triangles.IndexFormat					= DXGI_FORMAT_R32_UINT;
	GeometryDesc.Triangles.IndexBuffer					= InIndexBuffer->GetVirtualAddress();
	GeometryDesc.Triangles.IndexCount					= IndexCount;
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
	BufferProps.SizeInBytes		= Info.ScratchDataSizeInBytes;
	BufferProps.Flags			= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	BufferProps.InitalState		= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	BufferProps.HeapProperties	= HeapProps::DefaultHeap();

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
	AccelerationStructureDesc.DestAccelerationStructureData		= ResultBuffer->GetVirtualAddress();
	AccelerationStructureDesc.ScratchAccelerationStructureData	= ScratchBuffer->GetVirtualAddress();

	InCommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	InCommandList->UnorderedAccessBarrier(ResultBuffer->GetResource());

	// Keep instance of buffers
	VertexBuffer	= InVertexBuffer;
	IndexBuffer		= InIndexBuffer;
	
	return true;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RayTracingGeometry::GetVirtualAddress() const
{
	return ResultBuffer->GetVirtualAddress();
}

void D3D12RayTracingGeometry::SetName(const std::string& InName)
{
	ResultBuffer->SetName(InName);
}

/*
* RayTracingScene
*/

D3D12RayTracingScene::D3D12RayTracingScene(D3D12Device* Device)
	: D3D12DeviceChild(Device)
	, Instances()
{
}

D3D12RayTracingScene::~D3D12RayTracingScene()
{
	SAFEDELETE(ScratchBuffer);
	SAFEDELETE(ResultBuffer);
	SAFEDELETE(InstanceBuffer);
}

bool D3D12RayTracingScene::Initialize(D3D12CommandList* InCommandList, std::vector<D3D12RayTracingGeometryInstance>& InInstances)
{
	const Uint32 InstanceCount = static_cast<Uint32>(InInstances.size());

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
	BufferProps.SizeInBytes		= Info.ScratchDataSizeInBytes;
	BufferProps.Flags			= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	BufferProps.InitalState		= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	BufferProps.HeapProperties	= HeapProps::DefaultHeap();

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

	BufferProps.SizeInBytes		= sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * InstanceCount;
	BufferProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.InitalState		= D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.HeapProperties	= HeapProps::UploadHeap();

	InstanceBuffer = new D3D12Buffer(Device);
	if(!InstanceBuffer->Initialize(BufferProps))
	{
		return false;
	}

	// Map and set each instance matrix
	D3D12_RAYTRACING_INSTANCE_DESC* InstanceDesc = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(InstanceBuffer->Map());
	for (Uint32 i = 0; i < InstanceCount; i++)
	{
		InstanceDesc->InstanceID							= i;	// Exposed to the shader via InstanceID()
		InstanceDesc->InstanceContributionToHitGroupIndex	= 0;	// Offset inside the shader-table. We only have a single geometry, so the offset 0
		InstanceDesc->Flags									= D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		D3D12RayTracingGeometryInstance& Instance = InInstances[i];
		InstanceDesc->AccelerationStructure		= Instance.Geometry->GetVirtualAddress();
		InstanceDesc->InstanceMask				= 0xFF;

		memcpy(InstanceDesc->Transform, &Instance.Transform, sizeof(InstanceDesc->Transform));

		InstanceDesc++;
	}

	// Unmap
	InstanceBuffer->Unmap();

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
	AccelerationStructureDesc.Inputs							= Inputs;
	AccelerationStructureDesc.Inputs.InstanceDescs				= InstanceBuffer->GetVirtualAddress();
	AccelerationStructureDesc.DestAccelerationStructureData		= ResultBuffer->GetVirtualAddress();
	AccelerationStructureDesc.ScratchAccelerationStructureData	= ScratchBuffer->GetVirtualAddress();
	InCommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc);

	// UAV barrier needed before using the acceleration structures in a raytracing operation
	InCommandList->UnorderedAccessBarrier(ResultBuffer->GetResource());

	// Copy the instances
	Instances = InInstances;

	// Create descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.ViewDimension								= D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	SrvDesc.Shader4ComponentMapping						= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.RaytracingAccelerationStructure.Location	= ResultBuffer->GetVirtualAddress();

	CPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetCPUDescriptorHandleAt(1);
	Device->GetDevice()->CreateShaderResourceView(nullptr, &SrvDesc, CPUHandle);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RayTracingScene::GetVirtualAddress() const
{
	return ResultBuffer->GetVirtualAddress();
}

void D3D12RayTracingScene::SetName(const std::string& InName)
{
	ResultBuffer->SetName(InName);
}
