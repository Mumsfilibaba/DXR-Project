#pragma once
#include "Core/Core.h"

#include "RHI/RHITypes.h"

struct FMeshDrawCommand
{
    class FMaterial*  Material     = nullptr;
    class FMesh*      Mesh         = nullptr;
    class FActor*     CurrentActor = nullptr;

    class FRHIBuffer* VertexBuffer = nullptr;
    uint32            NumVertices  = 0;
    class FRHIBuffer* IndexBuffer  = nullptr;
    uint32            NumIndices   = 0;
    EIndexFormat      IndexFormat  = EIndexFormat::Unknown;

    class FRHIRayTracingGeometry* Geometry = nullptr;
};

MARK_AS_REALLOCATABLE(FMeshDrawCommand);