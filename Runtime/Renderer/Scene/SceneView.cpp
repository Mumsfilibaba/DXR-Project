#include "Engine/World/Camera.h"
#include "Renderer/RendererStats.h"
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
    if (MaxMeshes > 0 && static_cast<uint32>(StaticMeshes.Capacity()) < MaxMeshes)
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
    STAT_ADD(STAT_Render_ObjectsTested, 1);

    if (!Frustum)
    {
        MeshBatcher.AddStaticMesh(StaticMesh);
        STAT_ADD(STAT_Render_ObjectsVisible, 1);
        return true;
    }

    if (Frustum->IntersectsAABB(StaticMesh->GetWorldBounds()))
    {
        MeshBatcher.AddStaticMesh(StaticMesh);
        STAT_ADD(STAT_Render_ObjectsVisible, 1);
        return true;
    }
    else
    {
        STAT_ADD(STAT_Render_ObjectsCulled, 1);
        return false;
    }
}