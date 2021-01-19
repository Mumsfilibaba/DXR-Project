#include "Mesh.h"
#include "Renderer.h"

#include "Engine/EngineGlobals.h"

#include "RenderingCore/CommandList.h"
#include "RenderingCore/RenderLayer.h"

#include <algorithm>

/*
* Mesh
*/

bool Mesh::Init(const MeshData& Data)
{
	VertexCount = static_cast<UInt32>(Data.Vertices.Size());
	IndexCount	= static_cast<UInt32>(Data.Indices.Size());

	const UInt32 BufferUsage =
		RenderLayer::IsRayTracingSupported() ?
		BufferUsage_SRV | BufferUsage_Default :
		BufferUsage_Default;

	// Create VertexBuffer
	ResourceData InitialData(Data.Vertices.Data());
	VertexBuffer = RenderLayer::CreateVertexBuffer<Vertex>(
		&InitialData,
		VertexCount, 
		BufferUsage);
	if (!VertexBuffer)
	{
		return false;
	}

	// Create IndexBuffer
	InitialData = ResourceData(Data.Indices.Data());
	IndexBuffer = RenderLayer::CreateIndexBuffer(
		&InitialData,
		IndexCount * sizeof(UInt32), 
		EIndexFormat::IndexFormat_UInt32, 
		BufferUsage);
	if (!IndexBuffer)
	{
		return false;
	}

	// Create RaytracingGeometry if raytracing is supported
	if (RenderLayer::IsRayTracingSupported())
	{
		RayTracingGeometry = RenderLayer::CreateRayTracingGeometry();

		VertexBufferSRV = RenderLayer::CreateShaderResourceView<Vertex>(
			VertexBuffer.Get(), 
			0, 
			VertexCount);
		if (!VertexBufferSRV)
		{
			return false;
		}

		// ByteAddressBuffer
		IndexBufferSRV = RenderLayer::CreateShaderResourceView(
			IndexBuffer.Get(), 
			0, IndexCount);
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
	if (Result->Init(Data))
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
