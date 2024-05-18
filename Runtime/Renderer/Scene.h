#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"
#include "RendererCore/Interfaces/IScene.h"
#include "RHI/RHICore.h"

class FWorld;
class FMaterial;
class FDirectionalLight;
class FPointLight;

extern bool GFreezeRendering;

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

struct FScenePointLight
{
    struct FShadowData
    {
        FMatrix4 Matrix;
        FVector3 Position;
        float    NearPlane;
        float    FarPlane;
    };

    FScenePointLight(FPointLight* InLight)
        : Light(InLight)
    {
    }
    
    ~FScenePointLight()
    {
        Light = nullptr;
    }

    FPointLight*                  Light;
    FFrustum                      Frustums[RHI_NUM_CUBE_FACES];
    TArray<FMeshBatch>            MeshBatches[RHI_NUM_CUBE_FACES];
    TArray<FProxySceneComponent*> Primitives[RHI_NUM_CUBE_FACES];
    FShadowData                   ShadowData[RHI_NUM_CUBE_FACES];
    TArray<FMeshBatch>            SinglePassMeshBatch;
    TArray<FProxySceneComponent*> SinglePassPrimitives;
    TArray<FMeshBatch>            TwoPassMeshBatches[2];
    TArray<FProxySceneComponent*> TwoPassPrimitives[2];
};

struct FSceneDirectionalLight
{
    FSceneDirectionalLight(FDirectionalLight* InLight)
        : Light(InLight)
    {
    }

    ~FSceneDirectionalLight()
    {
        Light = nullptr;
    }

    FDirectionalLight*            Light;
    TArray<FMeshBatch>            MeshBatches;
    TArray<FProxySceneComponent*> Primitives;
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
    TArray<FLight*>           Lights;
    FSceneDirectionalLight*   DirectionalLight;
    TArray<FScenePointLight*> PointLights;

    // All materials
    TArray<FMaterial*> Materials;
};