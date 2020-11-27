#include "Mesh.h"

#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12DescriptorHeap.h"

#include "Renderer.h"

#include <algorithm>

Mesh::Mesh()
	: VertexBuffer(nullptr)
	, IndexBuffer(nullptr)
	, RayTracingGeometry(nullptr)
{
}

Mesh::~Mesh()
{
}

bool Mesh::Initialize(const MeshData& Data)
{
	// Create VertexBuffer
	BufferProperties BufferProps = { };
	BufferProps.SizeInBytes = Data.Vertices.Size() * sizeof(Vertex);
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.InitalState = D3D12_RESOURCE_STATE_COMMON;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;

	VertexBuffer = RenderingAPI::Get().CreateBuffer(BufferProps);
	if (!VertexBuffer)
	{
		return false;
	}

	// Create IndexBuffer
	BufferProps.SizeInBytes = Data.Indices.Size() * sizeof(UInt32);

	IndexBuffer = RenderingAPI::Get().CreateBuffer(BufferProps);
	if (!IndexBuffer)
	{
		return false;
	}

	VertexCount = static_cast<UInt32>(Data.Vertices.Size());
	IndexCount	= static_cast<UInt32>(Data.Indices.Size());

	// Upload data
	TSharedPtr<D3D12ImmediateCommandList> CommandList = RenderingAPI::StaticGetImmediateCommandList();
	CommandList->TransitionBarrier(VertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->TransitionBarrier(IndexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	
	UInt32 SizeInBytes = Data.Vertices.Size() * sizeof(Vertex);
	CommandList->UploadBufferData(VertexBuffer.Get(), 0, Data.Vertices.Data(), SizeInBytes);
	
	SizeInBytes = Data.Indices.Size() * sizeof(UInt32);
	CommandList->UploadBufferData(IndexBuffer.Get(), 0, Data.Indices.Data(), SizeInBytes);

	CommandList->TransitionBarrier(VertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	CommandList->TransitionBarrier(IndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	CommandList->Flush();
	CommandList->WaitForCompletion();

	// Create RaytracingGeometry if raytracing is supported
	if (RenderingAPI::Get().IsRayTracingSupported())
	{
		RayTracingGeometry = RenderingAPI::Get().CreateRayTracingGeometry();

		// Also create shaderresourceviews for the buffers
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
		SrvDesc.Format						= DXGI_FORMAT_UNKNOWN;
		SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
		SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Buffer.FirstElement			= 0;
		SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
		SrvDesc.Buffer.NumElements			= VertexCount;
		SrvDesc.Buffer.StructureByteStride	= sizeof(Vertex);

		VertexBuffer->SetShaderResourceView(TSharedPtr(RenderingAPI::Get().CreateShaderResourceView(VertexBuffer->GetResource(), &SrvDesc)), 0);

		SrvDesc.Format						= DXGI_FORMAT_R32_TYPELESS;
		SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
		SrvDesc.Buffer.NumElements			= static_cast<UInt32>(IndexCount);
		SrvDesc.Buffer.StructureByteStride	= 0;

		IndexBuffer->SetShaderResourceView(TSharedPtr(RenderingAPI::Get().CreateShaderResourceView(IndexBuffer->GetResource(), &SrvDesc)), 0);

		DescriptorTable = RenderingAPI::Get().CreateDescriptorTable(2);
		DescriptorTable->SetShaderResourceView(VertexBuffer->GetShaderResourceView(0).Get(), 0);
		DescriptorTable->SetShaderResourceView(IndexBuffer->GetShaderResourceView(0).Get(), 1);
		DescriptorTable->CopyDescriptors();
	}

	// Create AABB
	CreateBoundingBox(Data);
	return true;
}

bool Mesh::BuildAccelerationStructure(D3D12CommandList* CommandList)
{
	return RayTracingGeometry->BuildAccelerationStructure(CommandList, VertexBuffer, VertexCount, IndexBuffer, IndexCount);
}

TSharedPtr<Mesh> Mesh::Make(const MeshData& Data)
{
	TSharedPtr<Mesh> Result = MakeShared<Mesh>();
	if (Result->Initialize(Data))
	{
		return Result;
	}
	else
	{
		return TSharedPtr<Mesh>(nullptr);
	}
}

void Mesh::CreateBoundingBox(const MeshData& Data)
{
	constexpr Float Inf = std::numeric_limits<Float>::infinity();
	XMFLOAT3 Min = XMFLOAT3(Inf, Inf, Inf);
	XMFLOAT3 Max = XMFLOAT3(-Inf, -Inf, -Inf);

	for (const Vertex& Vertex : Data.Vertices)
	{
		// X
		Min.x = std::min<Float>(Min.x, Vertex.Position.x);
		Max.x = std::max<Float>(Max.x, Vertex.Position.x);
		// Y
		Min.y = std::min<Float>(Min.y, Vertex.Position.y);
		Max.y = std::max<Float>(Max.y, Vertex.Position.y);
		// Z
		Min.z = std::min<Float>(Min.z, Vertex.Position.z);
		Max.z = std::max<Float>(Max.z, Vertex.Position.z);
	}

	BoundingBox.Top		= Max;
	BoundingBox.Bottom	= Min;
}
