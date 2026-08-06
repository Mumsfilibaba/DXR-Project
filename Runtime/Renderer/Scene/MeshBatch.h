#pragma once
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Engine/Resources/Material.h"
#include "RendererCore/VertexDeclaration.h"

struct FSceneStaticMesh;

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

    FMeshBatch(FMaterial* InMaterial, const FVertexDeclaration& InDeclaration);
    ~FMeshBatch();
    
    void AddStaticMesh(FSceneStaticMesh* StaticMesh, int32 MaterialIndex);

    FMaterial*             Material;
    FVertexDeclaration     Declaration;
    EMaterialFlags         EffectiveMaterialFlags;
    TArray<FMeshReference> MeshReferences;
};

struct FMeshBatcher
{
    FMeshBatcher();
    ~FMeshBatcher();

    void AddStaticMesh(FSceneStaticMesh* StaticMesh);
    void Clear();

    TArray<FMeshBatch>  MeshBatches;
    TMap<uint64, int32> BatchLookup;
};