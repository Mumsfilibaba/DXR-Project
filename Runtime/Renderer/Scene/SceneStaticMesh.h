#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "RHI/RHITypes.h"
#include "Renderer/Scene/SceneObject.h"

class FMaterial;
class FMesh;
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

    // FSceneObject Interface
    virtual void Tick() override final;

    TSharedPtr<FMesh>     GetMesh() const { return Mesh; }
    TSharedPtr<FMaterial> GetMaterial(int32 Index = 0) const { return Materials.IsValidIndex(Index) ? Materials[Index] : nullptr; }

    uint32 GetNumMaterials() const
    {
        return Materials.Size();
    }

    const FAABB&                GetWorldBounds()         const { return WorldBounds; };
    const FTransformBufferHLSL& GetTransformShaderData() const { return TransformBuffer; }

    FRHIBuffer*  GetIndexBuffer() const { return IndexBuffer; }
    EIndexFormat GetIndexFormat() const { return IndexFormat; }

    FRHIRayTracingGeometry* GetRayTracingGeometry() const { return Geometry; }

private:
    // Reference to the Actor
    class FActor*         Actor;
    FStaticMeshComponent* MeshComponent;

    // TransformData for this object
    FTransformBufferHLSL TransformBuffer;

    // AABB in world-space
    FAABB WorldBounds;

    // Reference to the Mesh
    TSharedPtr<FMesh> Mesh;

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
