#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Vector3.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Tasks/TaskHandle.h"
#include "RendererCore/Interfaces/IScene.h"
#include "RHI/RHICore.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneProxyData.h"
#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneCamera.h"
#include "Renderer/Scene/SceneDirectionalLight.h"
#include "Renderer/Scene/ScenePointLight.h"
#include "Renderer/Scene/SceneSkyLight.h"
#include "Renderer/Scene/SceneSkybox.h"
#include "Renderer/Scene/SceneLightProbe.h"
#include "Renderer/Scene/SceneView.h"

class FWorld;
class FMaterial;
class FActor;
class FDirectionalLightComponent;
class FLightProbeComponent;
class FPointLightComponent;
class FSceneComponent;
class FSkyLightComponent;
class FSkyboxComponent;
class FStaticMeshComponent;

extern bool GFreezeRendering;

class FObjectIdentificationRegistry
{
public:
    FObjectIdentificationRegistry();
    ~FObjectIdentificationRegistry();

    FActor* Resolve(uint32 ObjectID) const;
    
    uint32 GetOrCreate(FActor* Actor);
    uint32 Get(FActor* Actor) const;
    
    void Remove(FActor* Actor);

private:
    TMap<FActor*, uint32> ActorToObjectID;
    TMap<uint32, FActor*> ObjectIDToActor;
    uint32                NextObjectID;
};

class FScene : public IScene
{
public:
    FScene(FWorld* InWorld);
    virtual ~FScene();

    // IScene interface
    virtual void Tick() override final;

    virtual void AddSceneComponent(FSceneComponent* InComponent) override final;
    virtual void RemoveSceneComponent(FSceneComponent* InComponent) override final;

    virtual FActor* GetActorByObjectID(uint32 ObjectID) const override final;

    // ObjectID allocation for editor highlighting/picking (main thread).
    virtual uint32 GetOrCreateObjectID(FActor* Actor) override final;
    virtual void RemoveActorObjectID(FActor* Actor) override final;

    uint32 GetObjectID(FActor* Actor) const;

    // Applies a marshalled per-frame batch to the proxies.
    void RenderThread_ApplyRenderUpdates(const FRenderUpdateBatch& Batch);

    // Performs frustum culling and builds visible primitive batches.
    void RenderThread_PrepareViewsForRendering();

    // Applies the view camera and batch collected by Tick(), runs culling, and reclaims retired proxies.
    void RenderThread_ApplyAndCull(const FCameraSnapshot& CameraSnapshot, bool bHasCamera);

    // Defers deletion of objects
    void DeferDeletion(FSceneObject* InObject);

    // Deletes enqueued objects
    void DeleteDeferredObjects();

    FSceneCamera* GetCamera() const
    {
        return Camera;
    }

    const FSceneView& GetCameraView() const
    {
        return CameraView;
    }

    const TArray<FSceneStaticMesh*>& GetStaticMeshes() const
    {
        return StaticMeshes;
    }

    const TArray<FScenePointLight*>& GetPointLights() const
    {
        return PointLights;
    }

    FSceneSkyLight* GetSkyLight() const
    {
        return SkyLight;
    }

    FSceneDirectionalLight* GetDirectionalLight() const
    {
        return DirectionalLight;
    }

    FSceneSkybox* GetSkybox() const
    {
        return Skybox;
    }

    const TArray<FMaterial*>& GetMaterials() const
    {
        return Materials;
    }

    const TArray<FSceneLightProbe*>& GetLightProbes() const
    {
        return LightProbes;
    }

private:

    // Reads the live sources and produces a render batch.
    FRenderUpdateBatch CollectRenderUpdates();

    void AddLightProbe(FLightProbeComponent* InLightProbe);
    void AddSkybox(FSkyboxComponent* InSkyboxComponent);
    void AddStaticMesh(FStaticMeshComponent* InMeshComponent);
    void AddDirectionalLight(FDirectionalLightComponent* InDirectionalLight);
    void AddPointLight(FPointLightComponent* InPointLight);
    void AddSkyLight(FSkyLightComponent* InSkyLight);

    void RemoveLightProbe(FLightProbeComponent* InLightProbe);
    void RemoveSkybox(FSkyboxComponent* InSkyboxComponent);
    void RemoveStaticMesh(FStaticMeshComponent* InMeshComponent);
    void RemoveDirectionalLight(FDirectionalLightComponent* InDirectionalLight);
    void RemovePointLight(FPointLightComponent* InPointLight);
    void RemoveSkyLight(FSkyLightComponent* InSkyLight);

    FWorld*                       World;
    FSceneCamera*                 Camera;
    FSceneView                    CameraView;
    TArray<FSceneStaticMesh*>     StaticMeshes;
    TArray<FScenePointLight*>     PointLights;
    FSceneSkyLight*               SkyLight;
    FSceneDirectionalLight*       DirectionalLight;
    FSceneSkybox*                 Skybox;
    TArray<FMaterial*>            Materials;
    TArray<FSceneLightProbe*>     LightProbes;
    TArray<FSceneObject*>         DeferredObjects;
    FCriticalSection              DeferredObjectsCS;
    TArray<FStaticMeshComponent*> StaticMeshSources;
    TArray<FPointLightComponent*> PointLightSources;
    TArray<FLightProbeComponent*> LightProbeSources;
    FDirectionalLightComponent*   DirectionalLightSource;
    FSkyLightComponent*           SkyLightSource;
    FSkyboxComponent*             SkyboxSource;
    FObjectIdentificationRegistry ObjectIDs;
    FRenderUpdateBatch            LatestBatch;
};
