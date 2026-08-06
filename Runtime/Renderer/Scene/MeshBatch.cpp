#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneStaticMesh.h"

FMeshBatch::FMeshBatch(FMaterial* InMaterial, const FVertexDeclaration& InDeclaration)
    : Material(InMaterial)
    , Declaration(InDeclaration)
    , EffectiveMaterialFlags(InMaterial->GetMaterialFlags() & FMaterial::GetSupportedMaterialFlags(InDeclaration))
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
    , BatchLookup()
{
}

FMeshBatcher::~FMeshBatcher() = default;

void FMeshBatcher::AddStaticMesh(FSceneStaticMesh* StaticMesh)
{
    const FVertexDeclaration& Declaration = StaticMesh->Mesh->GetVertexDeclaration();

    const int32 NumMaterials = StaticMesh->GetNumMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
    {
        TSharedPtr<FMaterial> Material = StaticMesh->GetMaterial(MaterialIndex);

        const uint64 BatchKey = (reinterpret_cast<uint64>(Material.Get()) << 8) | Declaration.GetID();

        int32 BatchIndex;
        if (int32* ExistingBatchIndex = BatchLookup.Find(BatchKey))
        {
            BatchIndex = *ExistingBatchIndex;
        }
        else
        {
            BatchIndex = MeshBatches.Size();
            MeshBatches.Emplace(Material.Get(), Declaration);

            BatchLookup.Add(BatchKey, BatchIndex);
        }

        MeshBatches[BatchIndex].AddStaticMesh(StaticMesh, MaterialIndex);
    }
}

void FMeshBatcher::Clear()
{
    MeshBatches.Clear();
    BatchLookup.Clear();
}