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
	VertexBuffer = CreateVertexBuffer<Vertex>(nullptr, VertexCount, BufferUsage_Default);
	if (!VertexBuffer)
	{
		return false;
	}

	// Create IndexBuffer
	IndexBuffer = CreateIndexBuffer(nullptr, IndexCount * sizeof(Uint32), EIndexFormat::IndexFormat_Uint32, BufferUsage_Default);
	if (!IndexBuffer)
	{
		return false;
	}

	// Create RaytracingGeometry if raytracing is supported
	if (EngineGlobals::RenderingAPI->IsRayTracingSupported())
	{
		RayTracingGeometry = CreateRayTracingGeometry();

		VertexBufferSRV = CreateShaderResourceView<Vertex>(VertexBuffer.Get(), 0, VertexCount);
		if (!VertexBufferSRV)
		{
			return false;
		}

		IndexBufferSRV = CreateShaderResourceView(IndexBuffer.Get(), 0, IndexCount, EFormat::Format_R32_Typeless);
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
	constexpr Float32 Inf = std::numeric_limits<Float32>::infinity();
	XMFLOAT3 Min = XMFLOAT3(Inf, Inf, Inf);
	XMFLOAT3 Max = XMFLOAT3(-Inf, -Inf, -Inf);

	for (const Vertex& Vertex : Data.Vertices)
	{
		// X
		Min.x = std::min<Float32>(Min.x, Vertex.Position.x);
		Max.x = std::max<Float32>(Max.x, Vertex.Position.x);
		// Y
		Min.y = std::min<Float32>(Min.y, Vertex.Position.y);
		Max.y = std::max<Float32>(Max.y, Vertex.Position.y);
		// Z
		Min.z = std::min<Float32>(Min.z, Vertex.Position.z);
		Max.z = std::max<Float32>(Max.z, Vertex.Position.z);
	}

	BoundingBox.Top		= Max;
	BoundingBox.Bottom	= Min;
}
