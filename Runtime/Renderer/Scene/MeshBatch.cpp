#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneStaticMesh.h"

FMeshBatch::FMeshBatch(FMaterial* InMaterial)
    : Material(InMaterial)
    , MeshReferences()
{
}

FMeshBatch::~FMeshBatch()
{
    Material = nullptr;
}

void FMeshBatch::AddStaticMesh(FSceneStaticMesh* StaticMesh, int32 MaterialIndex)
{
    FMeshReference& MeshReference = MeshReferences.Emplace();
    MeshReference.StaticMesh   = StaticMesh;
    MeshReference.SubMeshIndex = MaterialIndex;
    
    const FSubMesh& SubMesh = StaticMesh->Mesh->GetSubMesh(MaterialIndex);
    MeshReference.BaseVertex  = SubMesh.BaseVertex;
    MeshReference.StartIndex  = SubMesh.StartIndex;
    MeshReference.VertexCount = SubMesh.VertexCount;
    MeshReference.IndexCount  = SubMesh.IndexCount;
}

FMeshBatcher::FMeshBatcher()
    : MeshBatches()
    , MaterialToBatchIndex()
{
}

FMeshBatcher::~FMeshBatcher()
{
}

void FMeshBatcher::AddStaticMesh(FSceneStaticMesh* StaticMesh)
{
    const int32 NumMaterials = StaticMesh->GetNumMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
    {
        FMaterial* Material = StaticMesh->GetMaterial(MaterialIndex);

        int32 BatchIndex;
        if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(Material))
        {
            BatchIndex = *ExistingBatchIndex;
        }
        else
        {
            BatchIndex = MeshBatches.Size();
            MeshBatches.Emplace(Material);

            MaterialToBatchIndex.Add(Material, BatchIndex);
        }

        MeshBatches[BatchIndex].AddStaticMesh(StaticMesh, MaterialIndex);
    }
}

void FMeshBatcher::Clear()
{
    MeshBatches.Clear();
    MaterialToBatchIndex.Clear();
}