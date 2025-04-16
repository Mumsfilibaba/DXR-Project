#include "Engine/World/Camera.h"
#include "Renderer/Scene/SceneView.h"
#include "Renderer/Scene/SceneStaticMesh.h"

FSceneView::FSceneView()
    : Frustum(nullptr)
    , MeshBatcher()
    , StaticMeshes()
{
}

FSceneView::~FSceneView()
{
}

void FSceneView::PrepareView(uint32 MaxMeshes)
{
    // Prepare the array for a certain amount of meshes
    if (MaxMeshes > 0 && StaticMeshes.Capacity() < MaxMeshes)
    {
        StaticMeshes.Reserve(MaxMeshes);
    }

    // Clear the meshes and batches
    StaticMeshes.Clear();
    MeshBatcher.Clear();
}

void FSceneView::SetupFrustum(const FMatrix4& View, const FMatrix4& Projection)
{
    if (!Frustum)
    {
        Frustum = MakeUniquePtr<FFrustum>(View, Projection);
    }
    else
    {
        Frustum->Initialize(View, Projection);
    }
}

bool FSceneView::AddStaticMesh(FSceneStaticMesh* StaticMesh)
{
    // If we have no Frustum, just add the mesh
    if (!Frustum)
    {
        MeshBatcher.AddStaticMesh(StaticMesh);
        return true;
    }

    // If there are a frustum
    if (Frustum->IntersectsAABB(StaticMesh->GetWorldBounds()))
    {
        MeshBatcher.AddStaticMesh(StaticMesh);
        return true;
    }
    else
    {
        return false;
    }
}