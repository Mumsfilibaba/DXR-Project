#pragma once
#include "Core.h"

struct MeshDrawCommand
{
    class Material* Material     = nullptr;
    class Mesh*     Mesh         = nullptr;
    class Actor*    CurrentActor = nullptr;

    class VertexBuffer* VertexBuffer = nullptr;
    class IndexBuffer*  IndexBuffer  = nullptr;

    class RayTracingGeometry* Geometry = nullptr;
};