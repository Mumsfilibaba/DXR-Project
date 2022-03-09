#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMeshDrawCommand

struct SMeshDrawCommand
{
    class CMaterial*  Material      = nullptr;
    class CMesh*      Mesh          = nullptr;
    class CActor*     CurrentActor  = nullptr;
    class CRHIBuffer* VertexBuffer  = nullptr;
    class CRHIBuffer* IndexBuffer   = nullptr;
	uint32            NumIndices    = 0;

    class CRHIRayTracingGeometry* Geometry = nullptr;
};
