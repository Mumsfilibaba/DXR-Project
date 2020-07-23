#include "Mesh.h"

#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12UploadStack.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12DescriptorHeap.h"

#include <memory>

Mesh::Mesh()
	: VertexBuffer(nullptr)
	, IndexBuffer(nullptr)
	, RayTracingGeometry(nullptr)
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
	BufferProps.InitalState = D3D12_RESOURCE_STATE_COMMON;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;

	VertexBuffer = std::make_shared<D3D12Buffer>(Device);
	if (!VertexBuffer->Initialize(BufferProps))
	{
		return false;
	}

	// Create IndexBuffer
	BufferProps.SizeInBytes = Data.Indices.size() * sizeof(Uint32);

	IndexBuffer = std::make_shared<D3D12Buffer>(Device);
	if (!IndexBuffer->Initialize(BufferProps))
	{
		return false;
	}

	VertexCount = static_cast<Uint32>(Data.Vertices.size());
	IndexCount	= static_cast<Uint32>(Data.Indices.size());

	// Upload data
	std::unique_ptr<D3D12CommandAllocator> Allocator = std::unique_ptr<D3D12CommandAllocator>(new D3D12CommandAllocator(Device));
	if (!Allocator->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	std::unique_ptr<D3D12CommandList> CommandList = std::unique_ptr<D3D12CommandList>(new D3D12CommandList(Device));
	if (!CommandList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator.get(), nullptr))
	{
		return false;
	}

	std::unique_ptr<D3D12CommandQueue> Queue = std::unique_ptr<D3D12CommandQueue>(new D3D12CommandQueue(Device));
	if (!Queue->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	std::unique_ptr<D3D12UploadStack> UploadBuffer = std::make_unique<D3D12UploadStack>();
	if (!UploadBuffer->Initialize(Device))
	{
		return false;
	}

	Allocator->Reset();
	CommandList->Reset(Allocator.get());

	Uint32 Offset		= UploadBuffer->GetOffset();
	Uint32 SizeInBytes	= Data.Vertices.size() * sizeof(Vertex);
	void* BufferMemory	= UploadBuffer->Allocate(SizeInBytes);
	memcpy(BufferMemory, Data.Vertices.data(), SizeInBytes);

	CommandList->TransitionBarrier(VertexBuffer.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->CopyBuffer(VertexBuffer.get(), 0, UploadBuffer->GetBuffer(), Offset, SizeInBytes);

	Offset			= UploadBuffer->GetOffset();
	SizeInBytes		= Data.Indices.size() * sizeof(Uint32);
	BufferMemory	= UploadBuffer->Allocate(SizeInBytes);
	memcpy(BufferMemory, Data.Indices.data(), SizeInBytes);

	CommandList->TransitionBarrier(IndexBuffer.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->CopyBuffer(IndexBuffer.get(), 0, UploadBuffer->GetBuffer(), Offset, SizeInBytes);

	CommandList->TransitionBarrier(VertexBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	CommandList->TransitionBarrier(IndexBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	CommandList->Close();

	Queue->ExecuteCommandList(CommandList.get());
	Queue->WaitForCompletion();

	// Create RaytracingGeometry if raytracing is supported
	if (Device->IsRayTracingSupported())
	{
		RayTracingGeometry = std::make_shared<D3D12RayTracingGeometry>(Device);

		// Also create shaderresourceviews for the buffers
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
		SrvDesc.Format						= DXGI_FORMAT_UNKNOWN;
		SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
		SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Buffer.FirstElement			= 0;
		SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
		SrvDesc.Buffer.NumElements			= VertexCount;
		SrvDesc.Buffer.StructureByteStride	= sizeof(Vertex);

		VertexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device, VertexBuffer->GetResource(), &SrvDesc), 0);

		SrvDesc.Format						= DXGI_FORMAT_R32_TYPELESS;
		SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
		SrvDesc.Buffer.NumElements			= static_cast<Uint32>(IndexCount);
		SrvDesc.Buffer.StructureByteStride	= 0;

		IndexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device, IndexBuffer->GetResource(), &SrvDesc), 0);

		DescriptorTable = std::make_shared<D3D12DescriptorTable>(Device, 2);
		DescriptorTable->SetShaderResourceView(VertexBuffer->GetShaderResourceView(0).get(), 0);
		DescriptorTable->SetShaderResourceView(IndexBuffer->GetShaderResourceView(0).get(), 1);
		DescriptorTable->CopyDescriptors();
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
