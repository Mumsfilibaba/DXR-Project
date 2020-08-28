#pragma once
#include "MeshFactory.h"

#include "Containers/TSharedPtr.h"

#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12RayTracingScene.h"
#include "D3D12/D3D12CommandList.h"

/*
* AABB
*/
struct AABB
{
	XMFLOAT3 Top;
	XMFLOAT3 Bottom;

	FORCEINLINE Float32 GetWidth() const
	{
		return Top.x - Bottom.x;
	}

	FORCEINLINE Float32 GetHeight() const
	{
		return Top.y - Bottom.y;
	}

	FORCEINLINE Float32 GetDepth() const
	{
		return Top.z - Bottom.z;
	}
};

/*
* Mesh
*/
class Mesh
{
public:
	Mesh();
	~Mesh();

	bool Initialize(D3D12Device* Device, const MeshData& Data);
	
	bool BuildAccelerationStructure(D3D12CommandList* CommandList);

	static TSharedPtr<Mesh> Make(D3D12Device* Device, const MeshData& Data);

public:
	void CreateBoundingBox(const MeshData& Data);

	TSharedPtr<D3D12Buffer>				VertexBuffer;
	TSharedPtr<D3D12Buffer>				IndexBuffer;
	TSharedPtr<D3D12RayTracingGeometry>	RayTracingGeometry;
	TSharedPtr<D3D12DescriptorTable>	DescriptorTable;
	
	Uint32 VertexCount	= 0;
	Uint32 IndexCount	= 0;

	AABB BoundingBox;
};