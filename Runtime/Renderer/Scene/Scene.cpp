#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Frustum.h"
#include "Core/Threading/ScopedLock.h"
#include "Engine/World/World.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/SkyLight.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Renderer/RendererStats.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneSkybox.h"
#include "Renderer/Scene/SceneLightProbe.h"
#include "Renderer/Scene/SceneStaticMesh.h"

bool GFreezeRendering = false;

FScene::FScene(FWorld* InWorld)
    : IScene()
    , World(InWorld)
    , Camera(nullptr)
    , CameraView()
    , StaticMeshes()
    , PointLights()
    , SkyLight(nullptr)
    , DirectionalLight(nullptr)
    , Skybox(nullptr)
    , Materials()
    , DeferredObjects()
    , DeferredObjectsCS()
{
}

FScene::~FScene()
{
    // Delete all the objects that are deferred
    DeleteDeferredObjects();

    // StaticMeshes
    for (FSceneStaticMesh* StaticMesh : StaticMeshes)
    {
        SAFE_DELETE(StaticMesh);
    }

    StaticMeshes.Clear();

    // Lights
    for (FScenePointLight* ScenePointLight : PointLights)
    {
        SAFE_DELETE(ScenePointLight);
    }

    PointLights.Clear();

    // Light-Probes
    for (FSceneLightProbe* SceneLightProbe : LightProbes)
    {
        SAFE_DELETE(SceneLightProbe);
    }

    LightProbes.Clear();

    // Remove potential SkyLight
    SAFE_DELETE(SkyLight);

    // Remove potential DirectionalLight
    SAFE_DELETE(DirectionalLight);

    // Remove potential Skybox
    SAFE_DELETE(Skybox);

    // Reset Other stuff
    World  = nullptr;
    Camera = nullptr;

    STAT_SET(STAT_Scene_StaticMeshCount, 0);
    STAT_SET(STAT_Scene_PointLightCount, 0);
    STAT_SET(STAT_Scene_MaterialCount, 0);
    STAT_SET(STAT_Scene_LightProbeCount, 0);
}

void FScene::Tick()
{
    TRACE_SCOPE("Scene Tick");

    // Delete all the objects that are deferred
    DeleteDeferredObjects();

    if (GFreezeRendering)
    {
        return;
    }

    // Sync objects with the world
    SyncSceneAndWorld();

    // Performs frustum culling and updates visible primitives
    PrepareViewsForRendering();
}

void FScene::AddCamera(FCamera* InCamera)
{
    // TODO: For now it is replacing the current camera
    if (InCamera)
    {
        // TODO: Defer deletion
        Camera = InCamera;
    }
}

void FScene::AddLight(FLight* InLight)
{
    if (InLight)
    {
        if (FDirectionalLight* InDirectionalLight = Cast<FDirectionalLight>(InLight))
        {
            DeferDeletion(DirectionalLight);
            DirectionalLight = new FSceneDirectionalLight(this, InDirectionalLight);
        }
        else if (FSkyLight* InSkyLight = Cast<FSkyLight>(InLight))
        {
            DeferDeletion(SkyLight);

            SkyLight = new FSceneSkyLight(this, InSkyLight);
            SkyLight->FilterStaticCubeMaps();
        }
        else if (FPointLight* InPointLight = Cast<FPointLight>(InLight))
        {
            if (InPointLight->IsShadowCaster())
            {
                FScenePointLight* ScenePointLight = new FScenePointLight(this, InPointLight); 
                PointLights.Add(ScenePointLight);
                STAT_ADD(STAT_Scene_PointLightCount, 1);
            }
        }
    }
}

void FScene::AddLightProbe(FLightProbe* InLightProbe)
{
    if (InLightProbe)
    {
        FSceneLightProbe* NewLightProbe = new FSceneLightProbe(this, InLightProbe);
        NewLightProbe->FilterStaticCubeMaps();

        LightProbes.Add(NewLightProbe);
        STAT_ADD(STAT_Scene_LightProbeCount, 1);
    }
}

void FScene::AddSkybox(FSkyboxComponent* InSkyboxComponent)
{
    DeferDeletion(Skybox);

    if (InSkyboxComponent)
    {
        Skybox = new FSceneSkybox(this, InSkyboxComponent);
    }
}

void FScene::AddStaticMesh(FStaticMeshComponent* InMeshComponent)  
{
    if (InMeshComponent)
    {
        FSceneStaticMesh* NewStaticMesh = new FSceneStaticMesh(this, InMeshComponent);
        StaticMeshes.Add(NewStaticMesh);
        STAT_ADD(STAT_Scene_StaticMeshCount, 1);

        for (uint32 Index = 0; Index < NewStaticMesh->GetNumMaterials(); Index++)
        {
            CHECK(NewStaticMesh->GetMaterial(Index) != nullptr);
            Materials.AddUnique(NewStaticMesh->GetMaterial(Index).Get());
        }

        STAT_SET(STAT_Scene_MaterialCount, Materials.Size());
    }
}

uint32 FScene::GetOrCreateObjectID(FActor* Actor)
{
    if (!Actor)
    {
        return 0;
    }

    if (uint32* ExistingID = ActorToObjectID.Find(Actor))
    {
        return *ExistingID;
    }

    const uint32 NewID = NextObjectID;
    NextObjectID++;

    // Ensure 0 stays reserved for background.
    if (NextObjectID == 0)
    {
        NextObjectID = 1;
    }

    ActorToObjectID.Add(Actor, NewID);
    ObjectIDToActor.Add(NewID, Actor);
    return NewID;
}

uint32 FScene::GetObjectID(FActor* Actor) const
{
    if (!Actor)
    {
        return 0;
    }

    if (const uint32* ExistingID = ActorToObjectID.Find(Actor))
    {
        return *ExistingID;
    }

    return 0;
}

FActor* FScene::GetActorByObjectID(uint32 ObjectID) const
{
    if (ObjectID == 0)
    {
        return nullptr;
    }

    if (FActor* const* ExistingActor = ObjectIDToActor.Find(ObjectID))
    {
        return *ExistingActor;
    }

    return nullptr;
}

void FScene::SyncSceneAndWorld()
{
    TRACE_SCOPE("SyncSceneAndWorld");

    // Update StaticMeshes for the GPU (Matrices being in correct format etc)
    for (FSceneStaticMesh* StaticMesh : StaticMeshes)
    {
        StaticMesh->Tick();
    }

    // Update DirectionalLight
    if (DirectionalLight)
    {
        DirectionalLight->Tick();
    }

    // Update PointLights
    for (FScenePointLight* PointLight : PointLights)
    {
        PointLight->Tick();
    }

    // Update LightProbes
    for (FSceneLightProbe* LightProbe : LightProbes)
    {
        LightProbe->Tick();
    }
}

void FScene::PrepareViewsForRendering()
{
    TRACE_SCOPE("PrepareViewsForRendering");

    STAT_SET(STAT_Render_ObjectsTested, 0);
    STAT_SET(STAT_Render_ObjectsVisible, 0);
    STAT_SET(STAT_Render_ObjectsCulled, 0);

    // We shrink the array of meshes if it is too large
    if (StaticMeshes.Capacity() > StaticMeshes.Size())
    {
        StaticMeshes.Shrink();
    }

    // Prepare camera-view
    CameraView.PrepareView(StaticMeshes.Capacity());
    CameraView.SetupFrustum(Camera->GetViewMatrix(), Camera->GetProjectionMatrix());

    // Prepare directional-light shadow-view
    if (DirectionalLight)
    {
        DirectionalLight->GetShadowView().PrepareView(StaticMeshes.Capacity());
    }

    // Prepare point-light shadow-views
    for (FScenePointLight* PointLight : PointLights)
    {
        // Multi-pass (One pass per face)
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            PointLight->ShadowView[FaceIndex].PrepareView();

            // TODO: Move to light-update?
            PointLight->ShadowView[FaceIndex].SetupFrustum(PointLight->PointLight->GetViewMatrix(FaceIndex), PointLight->PointLight->GetProjectionMatrix(FaceIndex));
        }

        // Single Pass
        PointLight->SinglePassShadowView.PrepareView();
    }

    // Perform frustum culling
    for (FSceneStaticMesh* StaticMesh : StaticMeshes)
    {
        // Update camera-view visibility
        CameraView.AddStaticMesh(StaticMesh);

        // Update the visibility directional-light view
        if (DirectionalLight)
        {
            DirectionalLight->GetShadowView().AddStaticMesh(StaticMesh);
        }

        // Update the visibility PointLights
        for (FScenePointLight* PointLight : PointLights)
        {
            // Check if for each face if a primitive is visible...
            bool bIsVisibleSinglePass = false;
            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                if (PointLight->ShadowView[FaceIndex].AddStaticMesh(StaticMesh))
                {
                    bIsVisibleSinglePass = true;
                }
            }

            // ... if it is visible for any face we add it to the single-pass
            if (bIsVisibleSinglePass)
            {
                PointLight->SinglePassShadowView.AddStaticMesh(StaticMesh);
            }
        }
    }

#if STATS_ENABLED
    {
        const TArray<FMeshBatch>& Batches = CameraView.GetMeshBatches();
        STAT_SET(STAT_Render_MeshBatchCount, Batches.Size());

        int32 TotalRefs = 0;
        for (const FMeshBatch& Batch : Batches)
        {
            TotalRefs += Batch.MeshReferences.Size();
        }

        STAT_SET(STAT_Render_MeshReferenceCount, TotalRefs);
    }
#endif
}

void FScene::DeferDeletion(FSceneObject* InObject)
{
    if (InObject)
    {
        TScopedLock Lock(DeferredObjectsCS);
        DeferredObjects.Emplace(InObject);
    }
}

void FScene::DeleteDeferredObjects()
{
    TArray<FSceneObject*> LocalDeferredObjects;

    {
        TScopedLock Lock(DeferredObjectsCS);
        LocalDeferredObjects = Move(DeferredObjects);
    }

    for (FSceneObject* Object : LocalDeferredObjects)
    {
        SAFE_DELETE(Object);
    }
}
