#pragma once
#include "MeshFactory.h"

#include "Containers/TSharedPtr.h"

#include "RenderLayer/Buffer.h"
#include "RenderLayer/RayTracing.h"
#include "RenderLayer/CommandList.h"

#include "Scene/AABB.h"

class Mesh
{
public:
	bool Init(const MeshData& Data);
	
	bool BuildAccelerationStructure(CommandList& CmdList);

	static TSharedPtr<Mesh> Make(const MeshData& Data);

public:
	void CreateBoundingBox(const MeshData& Data);

	TSharedRef<VertexBuffer>		VertexBuffer;
	TSharedRef<ShaderResourceView>	VertexBufferSRV;
	TSharedRef<IndexBuffer>			IndexBuffer;
	TSharedRef<ShaderResourceView>	IndexBufferSRV;
	TSharedRef<RayTracingGeometry>	RayTracingGeometry;
	
	UInt32 VertexCount	= 0;
	UInt32 IndexCount	= 0;

	Float ShadowOffset = 0.0f;

	AABB BoundingBox;
};