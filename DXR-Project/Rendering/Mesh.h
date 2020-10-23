#pragma once
#include "MeshFactory.h"

#include "Containers/TSharedPtr.h"

#include "RenderingCore/Buffer.h"
#include "RenderingCore/RayTracing.h"
#include "RenderingCore/CommandList.h"

#include "Scene/AABB.h"

/*
* Mesh
*/

class Mesh
{
public:
	Mesh();
	~Mesh();

	bool Initialize(const MeshData& Data);
	
	bool BuildAccelerationStructure(CommandList& CmdList);

	static TSharedPtr<Mesh> Make(const MeshData& Data);

public:
	void CreateBoundingBox(const MeshData& Data);

	TSharedRef<VertexBuffer>		VertexBuffer;
	TSharedRef<ShaderResourceView>	VertexBufferSRV;
	TSharedRef<IndexBuffer>			IndexBuffer;
	TSharedRef<ShaderResourceView>	IndexBufferSRV;
	TSharedRef<RayTracingGeometry>	RayTracingGeometry;
	
	Uint32 VertexCount	= 0;
	Uint32 IndexCount	= 0;

	AABB BoundingBox;
};