#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Math/Matrix3x4.h"
#include "RHI/RHITypes.h"
#include "Renderer/Scene/SceneObject.h"

class FMaterial;
class FMesh;
class FStaticMeshComponent;
class FRHIBuffer;
class FRHIGeometryAccelerationStructure;

struct FTransformBufferHLSL
{
    FMatrix3x4 Transform     = {}; // Row-major float3x4 affine transform. Shaders treat positions as column vectors: result = Transform * float4(Position, 1).
    FMatrix3x4 TransformInvT = {}; // Inverse-transpose for normal/tangent transforms (w=0 so translation is ignored).
    uint32     ObjectID      = 0;
    uint32     Padding0      = 0;
    uint32     Padding1      = 0;
    uint32     Padding2      = 0;
};

MARK_AS_REALLOCATABLE(FTransformBufferHLSL);

class FSceneStaticMesh : public FSceneObject
{
public:
    FSceneStaticMesh(FScene* InScene, FStaticMeshComponent* MeshComponent);
    virtual ~FSceneStaticMesh();

    // FSceneObject Interface
    virtual void Tick() override final;

    TSharedPtr<FMesh>                  GetMesh()                    const { return Mesh; }
    TSharedPtr<FMaterial>              GetMaterial(int32 Index = 0) const { return Materials.IsValidIndex(Index) ? Materials[Index] : nullptr; }
    uint32                             GetNumMaterials()            const { return Materials.Size(); }
    const FAABB&                       GetWorldBounds()             const { return WorldBounds; };
    const FTransformBufferHLSL&        GetTransformShaderData()     const { return TransformBuffer; }
    FRHIBuffer*                        GetIndexBuffer()             const { return IndexBuffer; }
    EIndexFormat                       GetIndexFormat()             const { return IndexFormat; }
    FRHIGeometryAccelerationStructure* GetRayTracingGeometry()      const { return Geometry; }

private:
    class FActor*                      Actor;           // Reference to the Actor
    FStaticMeshComponent*              MeshComponent;
    FTransformBufferHLSL               TransformBuffer; // TransformData for this object
    FAABB                              WorldBounds;     // AABB in world-space
    TSharedPtr<FMesh>                  Mesh;            // Reference to the Mesh
    TArray<TSharedPtr<FMaterial>>      Materials;       // Reference to the material array
    FRHIGeometryAccelerationStructure* Geometry;        // Geometry Objects
    FRHIBuffer*                        VertexBuffer;
    FRHIBuffer*                        IndexBuffer;
    uint32                             NumVertices;
    uint32                             NumIndices;
    EIndexFormat                       IndexFormat;
};
