#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMeshDrawCommand

struct SMeshDrawCommand
{
    class FMaterial*              Material     = nullptr;
    class FMesh*                  Mesh         = nullptr;
    class FActor*                 CurrentActor = nullptr;

    class FRHIVertexBuffer*       VertexBuffer = nullptr;
    class FRHIIndexBuffer*        IndexBuffer  = nullptr;

    class FRHIRayTracingGeometry* Geometry     = nullptr;
};

template<>
struct TIsReallocatable<SMeshDrawCommand>
{
    enum
    {
        Value = true
    };
};