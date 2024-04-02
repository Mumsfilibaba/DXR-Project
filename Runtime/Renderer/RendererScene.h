#pragma once
#include "Core/Containers/Array.h"
#include "RendererCore/Interfaces/IRendererScene.h"

class FScene;
class FMaterial;
class FDirectionalLight;

struct FMeshBatch
{
    FMeshBatch(FMaterial* InMaterial)
        : Material(InMaterial)
        , Primitives()
    {
    }

    FMaterial*                       Material;
    TArray<FProxyRendererComponent*> Primitives;
};

class FRendererScene : public IRendererScene
{
public:
    FRendererScene(FScene* InScene);
    virtual ~FRendererScene();

    // Adds a camera to the scene
    virtual void AddCamera(FCamera* InCamera) override final;

    // Adds a light to the scene
    virtual void AddLight(FLight* InLight) override final;

    // TODO: Adds a new mesh to be drawn, but most renderer primitives should take this path
    virtual void AddProxyComponent(FProxyRendererComponent* InComponent) override final;

    // Performs frustum culling
    void UpdateVisibility();

    // Updates MeshBatches
    void UpdateBatches();

    // Scene that is mirrored by this RendererScene
    FScene* Scene;

    // TODO: Differ the Renderer's camera from the Scene's
    FCamera* Camera;

    // All Primitives in this scene
    TArray<FProxyRendererComponent*> Primitives;

    // Visible Primitives (From the camera's point of view)
    TArray<FProxyRendererComponent*> VisiblePrimitives;

    // Batches of meshes that are visible
    TArray<FMeshBatch> MeshBatches;

    // Batches of meshes that are visible
    TArray<FMeshBatch> VisibleMeshBatches;

    // All Lights in the Scene
    TArray<FLight*> Lights;

    // All materials
    TArray<FMaterial*> Materials;
};