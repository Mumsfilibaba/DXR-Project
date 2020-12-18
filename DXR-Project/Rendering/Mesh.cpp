#include "Mesh.h"
#include "Renderer.h"

#include "Engine/EngineGlobals.h"

#include "RenderingCore/CommandList.h"
#include "RenderingCore/RenderingAPI.h"

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
	VertexCount = static_cast<Uint32>(Data.Vertices.Size());
	IndexCount	= static_cast<Uint32>(Data.Indices.Size());

	// Create VertexBuffer
	VertexBuffer = RenderingAPI::CreateVertexBuffer<Vertex>(nullptr, VertexCount, BufferUsage_Default);
	if (!VertexBuffer)
	{
		return false;
	}

	// Create IndexBuffer
	IndexBuffer = RenderingAPI::CreateIndexBuffer(
		nullptr, 
		IndexCount * sizeof(UInt32), 
		EIndexFormat::IndexFormat_Uint32, 
		BufferUsage_Default);
	if (!IndexBuffer)
	{
		return false;
	}

	// Create RaytracingGeometry if raytracing is supported
	if (RenderingAPI::IsRayTracingSupported())
	{
		RayTracingGeometry = RenderingAPI::CreateRayTracingGeometry();

		VertexBufferSRV = RenderingAPI::CreateShaderResourceView<Vertex>(VertexBuffer.Get(), 0, VertexCount);
		if (!VertexBufferSRV)
		{
			return false;
		}

		IndexBufferSRV = RenderingAPI::CreateShaderResourceView(IndexBuffer.Get(), 0, IndexCount, EFormat::Format_R32_Typeless);
		if (!IndexBufferSRV)
		{
			return false;
		}
	}

	// Create AABB
	CreateBoundingBox(Data);
	return true;
}

bool Mesh::BuildAccelerationStructure(CommandList& CmdList)
{
	CmdList.BuildRayTracingGeometry(RayTracingGeometry.Get());
	return true;
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
