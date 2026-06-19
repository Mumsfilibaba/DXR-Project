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
struct FStaticMeshProxyUpdate;

struct FStaticMeshInitData
{
    TSharedPtr<FMesh>             Mesh;
    TArray<TSharedPtr<FMaterial>> Materials;
    uint32                        ObjectID = 0;
};

struct FTransformBufferHLSL
{
    Matrix3x4 Transform     = {}; // Row-major float3x4 affine transform. Shaders treat positions as column vectors: result = Transform * float4(Position, 1).
    Matrix3x4 TransformInvT = {}; // Inverse-transpose for normal/tangent transforms (w=0 so translation is ignored).
    uint32    ObjectID      = 0;
    uint32    Padding0      = 0;
    uint32    Padding1      = 0;
    uint32    Padding2      = 0;
};

MARK_AS_REALLOCATABLE(FTransformBufferHLSL);

struct FSceneStaticMesh : public FSceneObject
{
    FSceneStaticMesh(FScene* InScene, const FStaticMeshInitData& InitData);
    virtual ~FSceneStaticMesh();

    // Applies a per-frame transform snapshot (recomputes the GPU transform buffer + world bounds).
    void RenderThread_ApplyUpdate(const FStaticMeshProxyUpdate& Update);

    // Small helpers carrying bounds-check / count logic over the Materials array.
    TSharedPtr<FMaterial> GetMaterial(int32 Index = 0) const
    {
        return Materials.IsValidIndex(Index) ? Materials[Index] : nullptr;
    }
    
    uint32 GetNumMaterials() const
    {
        return Materials.Size();
    }

    FTransformBufferHLSL               TransformBuffer;
    FAABB                              WorldBounds; // AABB in world-space
    TSharedPtr<FMesh>                  Mesh;
    TArray<TSharedPtr<FMaterial>>      Materials;
    FRHIGeometryAccelerationStructure* Geometry;
    FRHIBuffer*                        VertexBuffer;
    FRHIBuffer*                        IndexBuffer;
    uint32                             NumVertices;
    uint32                             NumIndices;
    EIndexFormat                       IndexFormat;
};
