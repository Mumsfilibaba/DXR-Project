#pragma once
#include "Core/Core.h"
#include "RHI/RHITypes.h"

class FRHIBuffer;
class FRHIQuery;
class FRHIRayTracingGeometry;

#define NUM_OCCLUSION_QUERIES (3)

class FProxySceneComponent
{
public:
    FProxySceneComponent();
    virtual ~FProxySceneComponent();

    class FMaterial* Material;
    class FMesh*     Mesh;
    class FActor*    CurrentActor;

    FRHIRayTracingGeometry* Geometry;
    FRHIQuery*              CurrentOcclusionQuery;
    FRHIQuery*              OcclusionQueries[NUM_OCCLUSION_QUERIES];
    FRHIBuffer*             VertexBuffer;
    FRHIBuffer*             IndexBuffer;
    uint32                  NumVertices;
    uint32                  NumIndices;
    EIndexFormat            IndexFormat;
    uint32                  CurrentOcclusionQueryIndex;
};