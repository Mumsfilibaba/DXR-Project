#pragma once
#include "D3D12/D3D12Buffer.h"

#include "MeshFactory.h"

class Mesh
{
public:
	Mesh();
	~Mesh();

	static std::shared_ptr<Mesh> Make(D3D12Device* Device, const MeshData& Data);

public:
	std::shared_ptr<D3D12Buffer> VertexBuffer;
	std::shared_ptr<D3D12Buffer> IndexBuffer;
	Uint32 VertexCount	= 0;
	Uint32 IndexCount	= 0;
};