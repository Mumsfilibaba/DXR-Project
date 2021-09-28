#pragma once
#include "Core.h"

struct MeshDrawCommand
{
    class CMaterial* Material = nullptr;
    class Mesh* Mesh = nullptr;
    class CActor* CurrentActor = nullptr;

    class VertexBuffer* VertexBuffer = nullptr;
    class IndexBuffer* IndexBuffer = nullptr;

    class RayTracingGeometry* Geometry = nullptr;
};