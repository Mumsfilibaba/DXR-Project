#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"
#include "RendererCore/Interfaces/IScene.h"
#include "RHI/RHICore.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneDirectionalLight.h"
#include "Renderer/Scene/ScenePointLight.h"
#include "Renderer/Scene/SceneSkyLight.h"
#include "Renderer/Scene/SceneSkybox.h"
#include "Renderer/Scene/SceneLightProbe.h"

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

    // Adds a light-probe to the scene
    virtual void AddLightProbe(FLightProbe* InLightProbe) override final;

    // Adds a Skybox to the light
    virtual void AddSkybox(FSkyboxComponent* InSkyboxComponent) override final;

    // Adds a static mesh to the scene
    virtual void AddStaticMesh(FStaticMeshComponent* InMeshComponent) override final;

    // Update all scene objects with the world version of the object
    void SyncWithWorld();

    // Update Lights
    void UpdateLights();

    // Performs frustum culling
    void UpdateVisibility();

    // Updates primitives transform matrices to be ready for the GPU
    void UpdateStaticMeshes();

    // Updates MeshBatches
    void UpdateBatches();

    // Defers deletion of objects
    void DeferDeletion(FSceneObject* InObject);

    // Deletes enqueues objects
    void DeleteDeferredObjects();

    // World that is mirrored by this RendererScene
    FWorld* World;

    // TODO: Differ the Renderer's camera from the World's
    FCamera* Camera;

    // All static meshes in this scene
    TArray<FSceneStaticMesh*> StaticMeshes;

    // Visible static meshes (From the main camera's point of view)
    TArray<FSceneStaticMesh*> VisibleStaticMeshes;

    // Batches of meshes that are visible (From the main camera's point of view)
    TArray<FMeshBatch> VisibleMeshBatches;

    // All Lights in the Scene
    TArray<FLight*>           Lights;
    TArray<FScenePointLight*> PointLights;

    FSceneSkyLight*         SkyLight;
    FSceneDirectionalLight* DirectionalLight;

    // Pointer to skybox
    FSceneSkybox* Skybox;

    // All materials
    TArray<FMaterial*> Materials;

    // All LightProbes
    TArray<FSceneLightProbe*> LightProbes;

    // Objects to be deleted next frame
    TArray<FSceneObject*> DeferredObjects;
    FCriticalSection      DeferredObjectsCS;
};
