#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "RHI/RHITypes.h"
#include "Renderer/Scene/SceneObject.h"

class FMaterial;
class FStaticMeshComponent;
class FRHIBuffer;
class FRHIRayTracingGeometry;

struct FTransformBufferHLSL
{
    FMatrix4 Transform;
    FMatrix4 TransformInv;
};

MARK_AS_REALLOCATABLE(FTransformBufferHLSL);

class FSceneStaticMesh : public FSceneObject
{
public:
    FSceneStaticMesh(FScene* InScene, FStaticMeshComponent* MeshComponent);
    virtual ~FSceneStaticMesh();

    virtual void Tick() override final;

    FMaterial* GetMaterial(int32 Index = 0) const
    {
        return Materials.IsValidIndex(Index) ? Materials[Index].Get() : nullptr;
    }
    
    int32 GetNumMaterials() const
    {
        return Materials.Size();
    }
    
    // Reference to the Actor
    class FActor*         Actor;
    FStaticMeshComponent* MeshComponent;

    // TransformMatrix for this object
    FTransformBufferHLSL TransformBuffer;

    // AABB in world-space
    FAABB WorldBounds;

    // Reference to the Mesh
    TSharedPtr<class FMesh> Mesh;

    // Reference to the material array
    TArray<TSharedPtr<FMaterial>> Materials;

    // Geometry Objects
    FRHIRayTracingGeometry* Geometry;
    FRHIBuffer*             VertexBuffer;
    FRHIBuffer*             IndexBuffer;
    uint32                  NumVertices;
    uint32                  NumIndices;
    EIndexFormat            IndexFormat;
};
