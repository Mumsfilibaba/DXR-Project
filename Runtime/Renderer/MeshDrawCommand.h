#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshDrawCommand

struct FMeshDrawCommand
{
    class FMaterial*              Material     = nullptr;
    class FMesh*                  Mesh         = nullptr;
    class FActor*                 CurrentActor = nullptr;

    class FRHIVertexBuffer*       VertexBuffer = nullptr;
    class FRHIIndexBuffer*        IndexBuffer  = nullptr;

    class FRHIRayTracingGeometry* Geometry     = nullptr;
};

MARK_AS_REALLOCATABLE(FMeshDrawCommand);