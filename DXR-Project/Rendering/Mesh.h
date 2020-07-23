#pragma once
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12RayTracingScene.h"
#include "D3D12/D3D12CommandList.h"

#include "MeshFactory.h"

class Mesh
{
public:
	Mesh();
	~Mesh();

	bool Initialize(D3D12Device* Device, const MeshData& Data);
	
	bool BuildAccelerationStructure(D3D12CommandList* CommandList);

	static std::shared_ptr<Mesh> Make(D3D12Device* Device, const MeshData& Data);

public:
	std::shared_ptr<D3D12Buffer>				VertexBuffer;
	std::shared_ptr<D3D12Buffer>				IndexBuffer;
	std::shared_ptr<D3D12RayTracingGeometry>	RayTracingGeometry;
	std::shared_ptr<D3D12DescriptorTable>		DescriptorTable;
	
	Uint32 VertexCount	= 0;
	Uint32 IndexCount	= 0;
};