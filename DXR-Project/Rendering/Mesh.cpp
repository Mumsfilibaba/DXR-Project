#include "Mesh.h"

Mesh::Mesh()
	: VertexBuffer(nullptr)
	, IndexBuffer(nullptr)
{
}

Mesh::~Mesh()
{
}

std::shared_ptr<Mesh> Mesh::Make(D3D12Device* Device, const MeshData& Data)
{
	// Create Mesh
	std::shared_ptr<Mesh> Result = std::make_shared<Mesh>();
	
	// Create VertexBuffer
	BufferProperties BufferProps = { };
	BufferProps.SizeInBytes = Data.Vertices.size() * sizeof(Vertex);
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.InitalState = D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;
	
	Result->VertexBuffer = std::make_shared<D3D12Buffer>(Device);
	if (Result->VertexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = Result->VertexBuffer->Map();
		memcpy(BufferMemory, Data.Vertices.data(), BufferProps.SizeInBytes);
		Result->VertexBuffer->Unmap();
	}
	else
	{
		return std::shared_ptr<Mesh>(nullptr);
	}

	// Create IndexBuffer
	BufferProps.SizeInBytes = Data.Indices.size() * sizeof(Uint32);

	Result->IndexBuffer = std::make_shared<D3D12Buffer>(Device);
	if (Result->IndexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = Result->IndexBuffer->Map();
		memcpy(BufferMemory, Data.Indices.data(), BufferProps.SizeInBytes);
		Result->IndexBuffer->Unmap();
	}
	else
	{
		return std::shared_ptr<Mesh>(nullptr);
	}

	Result->VertexCount	= static_cast<Uint32>(Data.Vertices.size());
	Result->IndexCount	= static_cast<Uint32>(Data.Indices.size());

	return Result;
}
