#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"
#include "RendererCore/Interfaces/IScene.h"
#include "RHI/RHICore.h"
#include "RendererCore/Interfaces/ISceneObject.h"
#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneLights.h"
#include "Renderer/Scene/SceneSkybox.h"

class FWorld;
class FMaterial;
class FDirectionalLight;
class FPointLight;
class FSkyLight;

extern bool GFreezeRendering;

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

    // Adds a Skybox to the light
    virtual void AddSkybox(FSkyboxComponent* InSkyboxComponent) override final;

    // TODO: Adds a new mesh to be drawn, but most renderer primitives should take this path
    virtual void AddProxyComponent(FProxySceneComponent* InComponent) override final;

    // Update Lights
    void UpdateLights();

    // Performs frustum culling
    void UpdateVisibility();

    // Updates primitives transform matrices to be ready for the GPU
    void UpdatePrimitives();

    // Updates MeshBatches
    void UpdateBatches();

    // Defers deletion of objects
    void DeferDeletion(ISceneObject* InObject);

    // Deletes enqueues objects
    void DeleteDeferredObjects();

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
    TArray<FScenePointLight*> PointLights;

    FSceneSkyLight*           SkyLight;
    FSceneDirectionalLight*   DirectionalLight;

    // Pointer to skybox
    FSceneSkybox* Skybox;

    // All materials
    TArray<FMaterial*> Materials;

    // Objects to be deleted next frame
    TArray<ISceneObject*> DeferredObjects;
    FCriticalSection      DeferredObjectsCS;
};
