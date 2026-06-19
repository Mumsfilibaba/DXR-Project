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
class FDirectionalLight;
class FPointLight;
class FSkyLight;

extern bool GFreezeRendering;

class FObjectIDRegistry
{
public:
    FObjectIDRegistry();
    ~FObjectIDRegistry();

    FActor* Resolve(uint32 ObjectID) const;
    
    uint32 GetOrCreate(FActor* Actor);
    uint32 Get(FActor* Actor) const;

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
    
    virtual void AddCamera(FCamera* InCamera) override final;
    virtual void AddLight(FLight* InLight) override final;
    virtual void AddLightProbe(FLightProbe* InLightProbe) override final;
    virtual void AddSkybox(FSkyboxComponent* InSkyboxComponent) override final;
    virtual void AddStaticMesh(FStaticMeshComponent* InMeshComponent) override final;
    
    virtual void RemoveLight(FLight* InLight) override final;
    virtual void RemoveLightProbe(FLightProbe* InLightProbe) override final;
    virtual void RemoveStaticMesh(FStaticMeshComponent* InMeshComponent) override final;

    virtual FActor* GetActorByObjectID(uint32 ObjectID) const override final;

    // ObjectID allocation for editor highlighting/picking (main thread).
    virtual uint32 GetOrCreateObjectID(FActor* Actor) override final;

    uint32 GetObjectID(FActor* Actor) const;

    // Applies a marshalled per-frame batch to the proxies.
    void RenderThread_ApplyRenderUpdates(const FRenderUpdateBatch& Batch);

    // Performs frustum culling and builds visible primitive batches.
    void RenderThread_PrepareViewsForRendering();

    // Applies the batch collected by Tick(), runs culling, and reclaims retired proxies.
    void RenderThread_ApplyAndCull();

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
    FCamera*                      CameraSource;
    TArray<FStaticMeshComponent*> StaticMeshSources;
    TArray<FPointLight*>          PointLightSources;
    TArray<FLightProbe*>          LightProbeSources;
    FDirectionalLight*            DirectionalLightSource;
    FObjectIDRegistry             ObjectIDs;
    FRenderUpdateBatch            LatestBatch;
};
