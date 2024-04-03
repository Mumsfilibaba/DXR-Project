#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Frustum.h"
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

struct FLightView
{
    typedef TArray<FMeshBatch>               MeshBatchArray;
    typedef TArray<FProxyRendererComponent*> PrimitivesArray;

    enum ELightType : int32
    {
        LightType_None = 0,
        LightType_Directional,
        LightType_Point,
    };

    TArray<FFrustum>        Frustums;
    TArray<PrimitivesArray> Primitives;
    TArray<MeshBatchArray>  MeshBatches;
    ELightType              LightType;
    int32                   NumSubViews;
};

class FRendererScene : public IRendererScene
{
public:
    FRendererScene(FScene* InScene);
    virtual ~FRendererScene();

    // Update the scene this frame
    virtual void Tick() override final;

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

    // Visible Primitives (From the main camera's point of view)
    TArray<FProxyRendererComponent*> VisiblePrimitives;

    // Batches of meshes that are visible (From the main camera's point of view)
    TArray<FMeshBatch> VisibleMeshBatches;

    // All Lights in the Scene
    TArray<FLight*>    Lights;
    TArray<FLightView> LightViews;

    // NOTE: Currently a single DirectionalLight is supported
    int32 DirectionalLightIndex;

    // All materials
    TArray<FMaterial*> Materials;
};