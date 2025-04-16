#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Renderer/Scene/MeshBatch.h"

class FCamera;

class FSceneView
{
public:
    FSceneView();
    ~FSceneView();

    // Prepare the view for the next frame by clearing all the batched meshes
    void PrepareView(uint32 MaxMeshes = 0);

    // Creates the FFrustum for this view
    void SetupFrustum(const FMatrix4& View, const FMatrix4& Projection);

    // Add a static mesh to this view if the mesh is in view
    bool AddStaticMesh(FSceneStaticMesh* StaticMesh);

    const TArray<FMeshBatch>& GetMeshBatches() const
    {
        return MeshBatcher.MeshBatches;
    }

    const TArray<FSceneStaticMesh*>& GetStaticMeshes() const
    {
        return StaticMeshes;
    }

private:

    // Frustum for this view
    TUniquePtr<FFrustum> Frustum;

    // Batched static meshes
    FMeshBatcher MeshBatcher;

    // Visible static meshes
    TArray<FSceneStaticMesh*> StaticMeshes;
};