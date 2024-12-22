#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "RHI/RHITypes.h"

class FMaterial;
class FRHIBuffer;
class FRHIQuery;
class FRHIRayTracingGeometry;

#define NUM_OCCLUSION_QUERIES (3)
#define OCCLUSION_DELAY (6)

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

struct FTransformBufferHLSL
{
    FMatrix4 Transform;
    FMatrix4 TransformInv;
};

MARK_AS_REALLOCATABLE(FTransformBufferHLSL);

class ENGINE_API FProxySceneComponent
{
public:
    FProxySceneComponent();
    virtual ~FProxySceneComponent();

    void UpdateOcclusion();

    bool IsOccluded() const
    {
        return NumFramesOccluded > OCCLUSION_DELAY;
    }

    void UpdateFrustumVisbility(bool bIsVisible)
    {
        FrustumVisibility.bWasVisible = FrustumVisibility.bIsVisible;
        FrustumVisibility.bIsVisible  = bIsVisible;
    }

    FMaterial* GetMaterial(int32 Index = 0) const
    {
        return (Materials.Size() > Index) ? Materials[Index].Get() : nullptr;
    }
    
    int32 GetNumMaterials() const
    {
        return Materials.Size();
    }
    
    // Reference to the Actor
    class FActor* CurrentActor;

    // TransformMatrix for this object
    FTransformBufferHLSL TransformBuffer;

    // Reference to the Mesh
    TSharedPtr<class FMesh> Mesh;

    // Reference to the material array
    TArray<TSharedPtr<FMaterial>> Materials;
    
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
