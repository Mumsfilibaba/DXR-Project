#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"

class FSceneStaticMesh;
class FMaterial;

struct FMeshBatch
{
    struct FMeshReference
    {
        FMeshReference()
            : StaticMesh(nullptr)
            , SubMeshIndex(0)
            , BaseVertex(0)
            , VertexCount(0)
            , StartIndex(0)
            , IndexCount(0)
        {
        }

        FSceneStaticMesh* StaticMesh;
        int32             SubMeshIndex;
        uint32            BaseVertex;
        uint32            VertexCount;
        uint32            StartIndex;
        uint32            IndexCount;
    };

    FMeshBatch(FMaterial* InMaterial);
    ~FMeshBatch();
    
    void AddStaticMesh(FSceneStaticMesh* StaticMesh, int32 MaterialIndex);

    FMaterial*             Material;
    TArray<FMeshReference> MeshReferences;
};