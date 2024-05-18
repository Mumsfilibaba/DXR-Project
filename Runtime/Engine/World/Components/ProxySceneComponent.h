#pragma once
#include "Core/Core.h"
#include "RHI/RHITypes.h"

class FRHIBuffer;
class FRHIQuery;
class FRHIRayTracingGeometry;

#define NUM_OCCLUSION_QUERIES (3)

struct FFrustumVisibility
{
    FFrustumVisibility()
        : bIsVisible(false)
        , bWasVisible(false)
    {
    }

    bool bIsVisible  : 1;
    bool bWasVisible : 1;
};

class ENGINE_API FProxySceneComponent
{
public:
    FProxySceneComponent();
    virtual ~FProxySceneComponent();

    void UpdateOcclusion();

    bool IsOccluded() const
    {
        return NumFramesOccluded > NUM_OCCLUSION_QUERIES;
    }

    void UpdateFrustumVisbility(bool bIsVisible)
    {
        FrustumVisibility.bWasVisible = FrustumVisibility.bIsVisible;
        FrustumVisibility.bIsVisible = bIsVisible;
    }

    // Scene Objects
    class FMaterial*        Material;
    class FMesh*            Mesh;
    class FActor*           CurrentActor;
    
    // Occlusion
    FRHIQuery*              CurrentOcclusionQuery;
    FRHIQuery*              OcclusionQueries[NUM_OCCLUSION_QUERIES];
    uint32                  CurrentOcclusionQueryIndex;
    uint32                  NumFramesOccluded;
    FFrustumVisibility      FrustumVisibility;

    // Geometry Objects
    FRHIRayTracingGeometry* Geometry;
    FRHIBuffer*             VertexBuffer;
    FRHIBuffer*             IndexBuffer;
    uint32                  NumVertices;
    uint32                  NumIndices;
    EIndexFormat            IndexFormat;
};