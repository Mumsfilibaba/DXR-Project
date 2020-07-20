#include "Mesh.h"

#include "D3D12/D3D12Device.h"

Mesh::Mesh()
	: VertexBuffer(nullptr)
	, IndexBuffer(nullptr)
{
}

Mesh::~Mesh()
{
}

bool Mesh::Initialize(D3D12Device* Device, const MeshData& Data)
{
	// Create VertexBuffer
	BufferProperties BufferProps = { };
	BufferProps.SizeInBytes = Data.Vertices.size() * sizeof(Vertex);
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.InitalState = D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

	VertexBuffer = std::make_shared<D3D12Buffer>(Device);
	if (VertexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = VertexBuffer->Map();
		memcpy(BufferMemory, Data.Vertices.data(), BufferProps.SizeInBytes);
		VertexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	// Create IndexBuffer
	BufferProps.SizeInBytes = Data.Indices.size() * sizeof(Uint32);

	IndexBuffer = std::make_shared<D3D12Buffer>(Device);
	if (IndexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = IndexBuffer->Map();
		memcpy(BufferMemory, Data.Indices.data(), BufferProps.SizeInBytes);
		IndexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	VertexCount = static_cast<Uint32>(Data.Vertices.size());
	IndexCount	= static_cast<Uint32>(Data.Indices.size());

	// Create RaytracingGeometry if raytracing is supported
	if (Device->IsRayTracingSupported())
	{
		RayTracingGeometry = std::make_shared<D3D12RayTracingGeometry>(Device);
	}

	return true;
}

bool Mesh::BuildAccelerationStructure(D3D12CommandList* CommandList)
{
	return RayTracingGeometry->BuildAccelerationStructure(CommandList, VertexBuffer, VertexCount, IndexBuffer, IndexCount);
}

std::shared_ptr<Mesh> Mesh::Make(D3D12Device* Device, const MeshData& Data)
{
	std::shared_ptr<Mesh> Result = std::make_shared<Mesh>();
	if (Result->Initialize(Device, Data))
	{
		return Result;
	}
	else
	{
		return std::shared_ptr<Mesh>(nullptr);
	}
}
