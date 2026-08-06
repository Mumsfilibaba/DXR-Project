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

struct FPerObjectHLSL
{
    Matrix3x4 Transform       = {};   // Row-major float3x4 affine transform.
    Matrix3x4 TransformInvT   = {};   // Inverse-transpose for normal/tangent transforms.
    uint32    ObjectID        = 0;
    uint32    MaterialIndex   = 0;    // Index into the shared material StructuredBuffer (FMaterial::GetBufferIndex()).
    float     DeterminantSign = 1.0f; // -1 when the transform mirrors, which reverses tangent-space handedness.
    uint32    Padding2        = 0;
};

static_assert(sizeof(FPerObjectHLSL) == 112, "FPerObjectHLSL must match FPerObject in Structs.hlsli");

MARK_AS_REALLOCATABLE(FPerObjectHLSL);

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

    FPerObjectHLSL                     PerObjectBuffer;
    FAABB                              WorldBounds; // AABB in world-space
    TSharedPtr<FMesh>                  Mesh;
    TArray<TSharedPtr<FMaterial>>      Materials;
    FRHIGeometryAccelerationStructure* Geometry;
    FRHIBuffer*                        PositionBuffer;
    FRHIBuffer*                        IndexBuffer;
    uint32                             NumVertices;
    uint32                             NumIndices;
    EIndexFormat                       IndexFormat;
};
