#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"

class FProxySceneComponent;
class FMaterial;

struct FMeshBatch
{
    struct FMeshReference
    {
        FMeshReference()
            : Primitive(nullptr)
            , SubMeshIndex(0)
            , BaseVertex(0)
            , VertexCount(0)
            , StartIndex(0)
            , IndexCount(0)
        {
        }

        FProxySceneComponent* Primitive;
        int32                 SubMeshIndex;
        uint32                BaseVertex;
        uint32                VertexCount;
        uint32                StartIndex;
        uint32                IndexCount;
    };

    FMeshBatch(FMaterial* InMaterial);
    ~FMeshBatch();
    
    void AddPrimitive(FProxySceneComponent* Primitive, int32 MaterialIndex);

    FMaterial*             Material;
    TArray<FMeshReference> Primitives;
};