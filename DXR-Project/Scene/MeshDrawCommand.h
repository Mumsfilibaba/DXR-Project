#pragma once
#include "Defines.h"
#include "Types.h"

class D3D12Buffer;

struct MeshDrawCommand
{
	class Material* Material = nullptr;
	class Actor* CurrentActor = nullptr;
	
	D3D12Buffer* VertexBuffer	= nullptr;
	D3D12Buffer* IndexBuffer	= nullptr;

	Uint32 VertexCount	= 0;
	Uint32 IndexCount	= 0;

	class D3D12RayTracingGeometry* Geometry = nullptr;
};