#pragma once
#include "Core.h"

struct MeshDrawCommand
{
    class Material* Material     = nullptr;
    class Mesh*     Mesh         = nullptr;
    class Actor*    CurrentActor = nullptr;
    
    class VertexBuffer* VertexBuffer = nullptr;
    class IndexBuffer*  IndexBuffer  = nullptr;

    UInt32 VertexCount = 0;
    UInt32 IndexCount  = 0;

    class RayTracingGeometry* Geometry = nullptr;
};