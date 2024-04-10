#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"
#include "RendererCore/Interfaces/IScene.h"

class FWorld;
class FMaterial;
class FDirectionalLight;

struct FMeshBatch
{
    FMeshBatch(FMaterial* InMaterial)
        : Material(InMaterial)
        , Primitives()
    {
    }

    FMaterial*                    Material;
    TArray<FProxySceneComponent*> Primitives;
};

struct FLightView
{
    typedef TArray<FMeshBatch>            MeshBatchArray;
    typedef TArray<FProxySceneComponent*> PrimitivesArray;

    enum ELightType : int32
    {
        LightType_None = 0,
        LightType_Directional,
        LightType_Point,
    };

    struct FShadowData
    {
        FMatrix4 Matrix;
        FVector3 Position;
        float    NearPlane;
        float    FarPlane;
    };

    FLightView()
    {
    }

    FLightView(int32 NumSubViews)
    {
        Frustums.Resize(NumSubViews);
        Primitives.Resize(NumSubViews);
        MeshBatches.Resize(NumSubViews);
        ShadowData.Resize(NumSubViews);
    }

    TArray<FFrustum>        Frustums;
    TArray<FShadowData>     ShadowData;
    TArray<PrimitivesArray> Primitives;
    TArray<MeshBatchArray>  MeshBatches;
    ELightType              LightType;
    int32                   NumSubViews;
};

class FScene : public IScene
{
public:
    FScene(FWorld* InWorld);
    virtual ~FScene();

    // Update the scene this frame
    virtual void Tick() override final;

    // Adds a camera to the scene
    virtual void AddCamera(FCamera* InCamera) override final;

    // Adds a light to the scene
    virtual void AddLight(FLight* InLight) override final;

    // TODO: Adds a new mesh to be drawn, but most renderer primitives should take this path
    virtual void AddProxyComponent(FProxySceneComponent* InComponent) override final;

    // Update Lights
    void UpdateLights();

    // Performs frustum culling
    void UpdateVisibility();

    // Updates MeshBatches
    void UpdateBatches();

    // World that is mirrored by this RendererScene
    FWorld* World;

    // TODO: Differ the Renderer's camera from the World's
    FCamera* Camera;

    // All Primitives in this scene
    TArray<FProxySceneComponent*> Primitives;

    // Visible Primitives (From the main camera's point of view)
    TArray<FProxySceneComponent*> VisiblePrimitives;

    // Batches of meshes that are visible (From the main camera's point of view)
    TArray<FMeshBatch> VisibleMeshBatches;

    // All Lights in the Scene
    TArray<FLight*>     Lights;
    TArray<FLightView>  LightViews;
    TArray<FLightView*> PointLightViews;

    // NOTE: Currently a single DirectionalLight is supported
    int32 DirectionalLightIndex;

    // All materials
    TArray<FMaterial*> Materials;
};