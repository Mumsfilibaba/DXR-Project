#pragma once
#include "Core.h"

struct SMeshDrawCommand
{
    class CMaterial* Material = nullptr;
    class CMesh* Mesh = nullptr;
    class CActor* CurrentActor = nullptr;

    class CRHIVertexBuffer* VertexBuffer = nullptr;
    class CRHIIndexBuffer* IndexBuffer = nullptr;

    class CRHIRayTracingGeometry* Geometry = nullptr;
};