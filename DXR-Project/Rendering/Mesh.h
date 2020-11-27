#pragma once
#include "MeshFactory.h"

#include "Containers/TSharedPtr.h"

#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12RayTracingScene.h"
#include "D3D12/D3D12CommandList.h"

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
	
	bool BuildAccelerationStructure(D3D12CommandList* CommandList);

	static TSharedPtr<Mesh> Make(const MeshData& Data);

public:
	void CreateBoundingBox(const MeshData& Data);

	TSharedPtr<D3D12Buffer>				VertexBuffer;
	TSharedPtr<D3D12Buffer>				IndexBuffer;
	TSharedPtr<D3D12RayTracingGeometry>	RayTracingGeometry;
	TSharedPtr<D3D12DescriptorTable>	DescriptorTable;
	
	UInt32 VertexCount	= 0;
	UInt32 IndexCount	= 0;

	Float ShadowOffset = 0.0f;

	AABB BoundingBox;
};