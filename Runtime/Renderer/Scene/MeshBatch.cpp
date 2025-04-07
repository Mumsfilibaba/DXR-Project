#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneStaticMesh.h"

FMeshBatch::FMeshBatch(FMaterial* InMaterial)
    : Material(InMaterial)
    , MeshReferences()
{
}

FMeshBatch::~FMeshBatch()
{
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