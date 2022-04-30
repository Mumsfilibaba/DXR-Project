#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMeshDrawCommand

struct SMeshDrawCommand
{
    class CMaterial*              Material     = nullptr;
    class CMesh*                  Mesh         = nullptr;
    class CActor*                 CurrentActor = nullptr;

    class CRHIVertexBuffer*       VertexBuffer = nullptr;
    class CRHIIndexBuffer*        IndexBuffer  = nullptr;

    class CRHIRayTracingGeometry* Geometry     = nullptr;
};

template<>
struct TIsReallocatable<SMeshDrawCommand>
{
    enum
    {
        Value = true
    };
};