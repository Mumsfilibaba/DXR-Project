#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Math.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Tasks/Tasks.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Engine/World/Components/LightComponent.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/Components/PointLightComponent.h"
#include "Engine/World/Components/SceneComponent.h"
#include "Engine/World/Components/SkyLightComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/World/Components/SkyboxComponent.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Renderer/RendererStats.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"

bool GFreezeRendering = false;

FObjectIdentificationRegistry::FObjectIdentificationRegistry()
    : ActorToObjectID()
    , ObjectIDToActor()
    , NextObjectID(1)
{
}

FObjectIdentificationRegistry::~FObjectIdentificationRegistry() = default;

uint32 FObjectIdentificationRegistry::GetOrCreate(FActor* Actor)
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

uint32 FObjectIdentificationRegistry::Get(FActor* Actor) const
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

void FObjectIdentificationRegistry::Remove(FActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    if (uint32* ExistingID = ActorToObjectID.Find(Actor))
    {
        ObjectIDToActor.Remove(*ExistingID);
        ActorToObjectID.Remove(Actor);
    }
}

FActor* FObjectIdentificationRegistry::Resolve(uint32 ObjectID) const
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
    , LightProbes()
    , DeferredObjects()
    , DeferredObjectsCS()
    , StaticMeshSources()
    , PointLightSources()
    , LightProbeSources()
    , DirectionalLightSource(nullptr)
    , SkyLightSource(nullptr)
    , SkyboxSource(nullptr)
    , ObjectIDs()
    , LatestBatch()
{
    Camera = new FSceneCamera(this);
}

FScene::~FScene()
{
    // Drain any in-flight render-thread commands that captured `this` before tearing down proxies.
    Tasks::LaunchOnRenderThread("SceneFlush", []() {}).Wait();

    DeleteDeferredObjects();

    for (FSceneStaticMesh* StaticMesh : StaticMeshes)
    {
        SAFE_DELETE(StaticMesh);
    }

    StaticMeshes.Clear();

    for (FScenePointLight* ScenePointLight : PointLights)
    {
        SAFE_DELETE(ScenePointLight);
    }

    PointLights.Clear();

    for (FSceneLightProbe* SceneLightProbe : LightProbes)
    {
        SAFE_DELETE(SceneLightProbe);
    }

    LightProbes.Clear();

    SAFE_DELETE(SkyLight);
    SAFE_DELETE(DirectionalLight);
    SAFE_DELETE(Skybox);
    SAFE_DELETE(Camera);

    World                  = nullptr;
    DirectionalLightSource = nullptr;
    SkyLightSource         = nullptr;
    SkyboxSource           = nullptr;

    STAT_SET(STAT_Scene_StaticMeshCount, 0);
    STAT_SET(STAT_Scene_PointLightCount, 0);
    STAT_SET(STAT_Scene_MaterialCount, 0);
    STAT_SET(STAT_Scene_LightProbeCount, 0);
}

void FScene::Tick()
{
    TRACE_SCOPE("Scene Tick");
    CHECK_MAIN_THREAD();

    if (GFreezeRendering)
    {
        LatestBatch = FRenderUpdateBatch();
        return;
    }

    LatestBatch = CollectRenderUpdates();
}

void FScene::RenderThread_ApplyAndCull(const FCameraSnapshot& CameraSnapshot, bool bHasCamera)
{
    TRACE_SCOPE("Scene ApplyAndCull");
    CHECK_RENDER_THREAD();

    // Reclaim proxies retired by a previous frame, now that the GPU is done with them.
    DeleteDeferredObjects();

    if (GFreezeRendering)
    {
        return;
    }

    if (bHasCamera && Camera)
    {
        Camera->Snapshot = CameraSnapshot;
    }

    RenderThread_ApplyRenderUpdates(LatestBatch);
    RenderThread_PrepareViewsForRendering();
}

FRenderUpdateBatch FScene::CollectRenderUpdates()
{
    TRACE_SCOPE("CollectRenderUpdates");

    FRenderUpdateBatch Batch;

    // Static meshes
    Batch.StaticMeshUpdates.Reserve(StaticMeshSources.Size());
    for (FStaticMeshComponent* Component : StaticMeshSources)
    {
        FStaticMeshProxyUpdate Update;
        if (Component && Component->GetActorOwner())
        {
            const FActorTransform& Transform = Component->GetActorOwner()->GetWorldTransform();
            Update.TransformMatrix        = Transform.GetTransformMatrix();
            Update.TransformMatrixInverse = Transform.GetTransformMatrixInverse();
        }

        Batch.StaticMeshUpdates.Add(Update);
    }

    // Directional light
    if (DirectionalLightSource)
    {
        Batch.bHasDirectionalLight = true;
        
        FDirectionalLightProxyUpdate& Update = Batch.DirectionalLight;
        Update.Color                       = DirectionalLightSource->GetColor() * DirectionalLightSource->GetIntensity();
        Update.Direction                   = DirectionalLightSource->GetDirectionVector();
        Update.ShadowNearPlane             = DirectionalLightSource->GetShadowNearPlane();
        Update.ShadowFarPlane              = DirectionalLightSource->GetShadowFarPlane();
        Update.ShadowBias                  = DirectionalLightSource->GetShadowBias();
        Update.ShadowPositionOffset        = DirectionalLightSource->GetShadowPositionOffset();
        Update.CascadeSplitLambda          = DirectionalLightSource->GetCascadeSplitLambda();
        Update.LightArea                   = DirectionalLightSource->GetLightArea();
        Update.bCastShadows                = DirectionalLightSource->CastsShadows();
    }

    // Point lights
    Batch.PointLightUpdates.Reserve(PointLightSources.Size());
    for (FPointLightComponent* PointLight : PointLightSources)
    {
        FPointLightProxyUpdate Update;
        if (PointLight)
        {
            PointLight->UpdateShadowMatrices();
            Update.Position        = PointLight->GetPosition();
            Update.Color           = PointLight->GetColor() * PointLight->GetIntensity();
            Update.ShadowBias      = PointLight->GetShadowBias();
            Update.ShadowNearPlane = PointLight->GetShadowNearPlane();
            Update.ShadowFarPlane  = PointLight->GetShadowFarPlane();
            Update.bCastShadows    = PointLight->CastsShadows();

            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                Update.ViewMatrix[FaceIndex]     = PointLight->GetViewMatrix(FaceIndex);
                Update.ProjMatrix[FaceIndex]     = PointLight->GetProjectionMatrix(FaceIndex);
                Update.ViewProjMatrix[FaceIndex] = PointLight->GetViewProjectionMatrix(FaceIndex);
            }
        }

        Batch.PointLightUpdates.Add(Update);
    }

    // Light probes
    Batch.LightProbeUpdates.Reserve(LightProbeSources.Size());
    for (FLightProbeComponent* LightProbe : LightProbeSources)
    {
        FLightProbeProxyUpdate Update;
        if (LightProbe)
        {
            Update.Position       = LightProbe->GetPosition();
            Update.bBoxProjection = LightProbe->GetBoxProjection();
            Update.BoxOffset      = LightProbe->GetBoxOffset();
            Update.BoxExtent      = LightProbe->GetBoxExtents();
        }

        Batch.LightProbeUpdates.Add(Update);
    }

    return Batch;
}

void FScene::RenderThread_ApplyRenderUpdates(const FRenderUpdateBatch& Batch)
{
    TRACE_SCOPE("ApplyRenderUpdates");

    const int32 NumMeshUpdates = Math::Min(Batch.StaticMeshUpdates.Size(), StaticMeshes.Size());
    for (int32 Index = 0; Index < NumMeshUpdates; ++Index)
    {
        StaticMeshes[Index]->RenderThread_ApplyUpdate(Batch.StaticMeshUpdates[Index]);
    }

    if (Batch.bHasDirectionalLight && DirectionalLight && Camera)
    {
        DirectionalLight->RenderThread_ApplyUpdate(Batch.DirectionalLight, Camera->Snapshot.ViewProjectionInverse);
    }

    const int32 NumPointLightUpdates = Math::Min(Batch.PointLightUpdates.Size(), PointLights.Size());
    for (int32 Index = 0; Index < NumPointLightUpdates; ++Index)
    {
        PointLights[Index]->RenderThread_ApplyUpdate(Batch.PointLightUpdates[Index]);
    }

    const int32 NumLightProbeUpdates = Math::Min(Batch.LightProbeUpdates.Size(), LightProbes.Size());
    for (int32 Index = 0; Index < NumLightProbeUpdates; ++Index)
    {
        LightProbes[Index]->RenderThread_ApplyUpdate(Batch.LightProbeUpdates[Index]);
    }
}

void FScene::AddSceneComponent(FSceneComponent* InComponent)
{
    if (!InComponent)
    {
        return;
    }

    if (FStaticMeshComponent* StaticMesh = Cast<FStaticMeshComponent>(InComponent))
    {
        AddStaticMesh(StaticMesh);
    }
    else if (FSkyboxComponent* SkyboxComponent = Cast<FSkyboxComponent>(InComponent))
    {
        AddSkybox(SkyboxComponent);
    }
    else if (FLightProbeComponent* LightProbe = Cast<FLightProbeComponent>(InComponent))
    {
        AddLightProbe(LightProbe);
    }
    else if (FDirectionalLightComponent* DirectionalLightComponent = Cast<FDirectionalLightComponent>(InComponent))
    {
        AddDirectionalLight(DirectionalLightComponent);
    }
    else if (FSkyLightComponent* SkyLightComponent = Cast<FSkyLightComponent>(InComponent))
    {
        AddSkyLight(SkyLightComponent);
    }
    else if (FPointLightComponent* PointLightComponent = Cast<FPointLightComponent>(InComponent))
    {
        AddPointLight(PointLightComponent);
    }
}

void FScene::AddDirectionalLight(FDirectionalLightComponent* InDirectionalLight)
{
    if (!InDirectionalLight)
    {
        return;
    }

    if (DirectionalLightSource == InDirectionalLight)
    {
        return;
    }

    DirectionalLightSource = InDirectionalLight;
    Tasks::LaunchOnRenderThread("AddDirectionalLight", [this]()
    {
        DeferDeletion(DirectionalLight);
        DirectionalLight = new FSceneDirectionalLight(this);
    });
}

void FScene::AddSkyLight(FSkyLightComponent* InSkyLight)
{
    if (!InSkyLight || !InSkyLight->GetCubeMap())
    {
        return;
    }

    if (SkyLightSource == InSkyLight)
    {
        return;
    }

    SkyLightSource = InSkyLight;

    const FRHITextureRef CubeMap = InSkyLight->GetCubeMap();
    Tasks::LaunchOnRenderThread("AddSkyLight", [this, CubeMap]()
    {
        DeferDeletion(SkyLight);
        SkyLight = new FSceneSkyLight(this, CubeMap);
        SkyLight->RenderThread_FilterStaticCubeMaps();
    });
}

void FScene::AddPointLight(FPointLightComponent* InPointLight)
{
    if (!InPointLight)
    {
        return;
    }

    if (PointLightSources.IsValidIndex(PointLightSources.Find(InPointLight)))
    {
        return;
    }

    PointLightSources.Add(InPointLight);
    Tasks::LaunchOnRenderThread("AddPointLight", [this]()
    {
        PointLights.Add(new FScenePointLight(this));
        STAT_ADD(STAT_Scene_PointLightCount, 1);
    });
}

void FScene::AddLightProbe(FLightProbeComponent* InLightProbe)
{
    if (!InLightProbe || !InLightProbe->GetCubeMap())
    {
        return;
    }

    if (LightProbeSources.IsValidIndex(LightProbeSources.Find(InLightProbe)))
    {
        return;
    }

    LightProbeSources.Add(InLightProbe);

    FRHITextureRef CubeMap = InLightProbe->GetCubeMap();
    Tasks::LaunchOnRenderThread("AddLightProbe", [this, CubeMap]()
    {
        FSceneLightProbe* NewLightProbe = new FSceneLightProbe(this, CubeMap);
        NewLightProbe->RenderThread_FilterStaticCubeMaps();
        LightProbes.Add(NewLightProbe);
        STAT_ADD(STAT_Scene_LightProbeCount, 1);
    });
}

void FScene::AddSkybox(FSkyboxComponent* InSkyboxComponent)
{
    if (SkyboxSource == InSkyboxComponent)
    {
        return;
    }

    SkyboxSource = InSkyboxComponent;

    const FRHITextureRef CubeMap = InSkyboxComponent ? InSkyboxComponent->GetCubeMap() : FRHITextureRef(nullptr);
    Tasks::LaunchOnRenderThread("AddSkybox", [this, CubeMap]()
    {
        DeferDeletion(Skybox);
        Skybox = CubeMap ? new FSceneSkybox(this, CubeMap) : nullptr;
    });
}

void FScene::AddStaticMesh(FStaticMeshComponent* InMeshComponent)
{
    if (!InMeshComponent)
    {
        return;
    }

    if (StaticMeshSources.IsValidIndex(StaticMeshSources.Find(InMeshComponent)))
    {
        return;
    }

    StaticMeshSources.Add(InMeshComponent);

    FStaticMeshInitData InitData;
    InitData.Mesh      = InMeshComponent->GetMesh();
    InitData.Materials = InMeshComponent->GetMaterials();
    InitData.ObjectID  = ObjectIDs.GetOrCreate(InMeshComponent->GetActorOwner());

    Tasks::LaunchOnRenderThread("AddStaticMesh", [this, InitData]()
    {
        FSceneStaticMesh* NewStaticMesh = new FSceneStaticMesh(this, InitData);
        StaticMeshes.Add(NewStaticMesh);
        STAT_ADD(STAT_Scene_StaticMeshCount, 1);

        for (uint32 Index = 0; Index < NewStaticMesh->GetNumMaterials(); Index++)
        {
            CHECK(NewStaticMesh->GetMaterial(Index) != nullptr);
            Materials.AddUnique(NewStaticMesh->GetMaterial(Index).Get());
        }

        STAT_SET(STAT_Scene_MaterialCount, Materials.Size());
    });
}

void FScene::RemoveSceneComponent(FSceneComponent* InComponent)
{
    if (!InComponent)
    {
        return;
    }

    if (FStaticMeshComponent* StaticMesh = Cast<FStaticMeshComponent>(InComponent))
    {
        RemoveStaticMesh(StaticMesh);
    }
    else if (FSkyboxComponent* SkyboxComponent = Cast<FSkyboxComponent>(InComponent))
    {
        RemoveSkybox(SkyboxComponent);
    }
    else if (FLightProbeComponent* LightProbe = Cast<FLightProbeComponent>(InComponent))
    {
        RemoveLightProbe(LightProbe);
    }
    else if (FDirectionalLightComponent* DirectionalLightComponent = Cast<FDirectionalLightComponent>(InComponent))
    {
        RemoveDirectionalLight(DirectionalLightComponent);
    }
    else if (FSkyLightComponent* SkyLightComponent = Cast<FSkyLightComponent>(InComponent))
    {
        RemoveSkyLight(SkyLightComponent);
    }
    else if (FPointLightComponent* PointLightComponent = Cast<FPointLightComponent>(InComponent))
    {
        RemovePointLight(PointLightComponent);
    }
}

void FScene::RemoveDirectionalLight(FDirectionalLightComponent* InDirectionalLight)
{
    if (DirectionalLightSource != InDirectionalLight)
    {
        return;
    }

    DirectionalLightSource = nullptr;
    Tasks::LaunchOnRenderThread("RemoveDirectionalLight", [this]()
    {
        DeferDeletion(DirectionalLight);
        DirectionalLight = nullptr;
    });
}

void FScene::RemoveSkyLight(FSkyLightComponent* InSkyLight)
{
    if (SkyLightSource != InSkyLight)
    {
        return;
    }

    SkyLightSource = nullptr;
    Tasks::LaunchOnRenderThread("RemoveSkyLight", [this]()
    {
        DeferDeletion(SkyLight);
        SkyLight = nullptr;
    });
}

void FScene::RemovePointLight(FPointLightComponent* InPointLight)
{
    const int32 Index = PointLightSources.Find(InPointLight);
    if (!PointLightSources.IsValidIndex(Index))
    {
        return;
    }

    PointLightSources.RemoveAt(Index);
    Tasks::LaunchOnRenderThread("RemovePointLight", [this, Index]()
    {
        if (PointLights.IsValidIndex(Index))
        {
            DeferDeletion(PointLights[Index]);
            PointLights.RemoveAt(Index);
            STAT_ADD(STAT_Scene_PointLightCount, -1);
        }
    });
}

void FScene::RemoveLightProbe(FLightProbeComponent* InLightProbe)
{
    const int32 Index = LightProbeSources.Find(InLightProbe);
    if (!LightProbeSources.IsValidIndex(Index))
    {
        return;
    }

    LightProbeSources.RemoveAt(Index);
    Tasks::LaunchOnRenderThread("RemoveLightProbe", [this, Index]()
    {
        if (LightProbes.IsValidIndex(Index))
        {
            DeferDeletion(LightProbes[Index]);
            LightProbes.RemoveAt(Index);
            STAT_ADD(STAT_Scene_LightProbeCount, -1);
        }
    });
}

void FScene::RemoveSkybox(FSkyboxComponent* InSkyboxComponent)
{
    if (SkyboxSource != InSkyboxComponent)
    {
        return;
    }

    SkyboxSource = nullptr;
    Tasks::LaunchOnRenderThread("RemoveSkybox", [this]()
    {
        DeferDeletion(Skybox);
        Skybox = nullptr;
    });
}

void FScene::RemoveStaticMesh(FStaticMeshComponent* InMeshComponent)
{
    const int32 Index = StaticMeshSources.Find(InMeshComponent);
    if (!StaticMeshSources.IsValidIndex(Index))
    {
        return;
    }

    StaticMeshSources.RemoveAt(Index);
    Tasks::LaunchOnRenderThread("RemoveStaticMesh", [this, Index]()
    {
        if (StaticMeshes.IsValidIndex(Index))
        {
            DeferDeletion(StaticMeshes[Index]);
            StaticMeshes.RemoveAt(Index);
            STAT_ADD(STAT_Scene_StaticMeshCount, -1);
        }
    });
}

uint32 FScene::GetOrCreateObjectID(FActor* Actor)
{
    return ObjectIDs.GetOrCreate(Actor);
}

void FScene::RemoveActorObjectID(FActor* Actor)
{
    ObjectIDs.Remove(Actor);
}

uint32 FScene::GetObjectID(FActor* Actor) const
{
    return ObjectIDs.Get(Actor);
}

FActor* FScene::GetActorByObjectID(uint32 ObjectID) const
{
    return ObjectIDs.Resolve(ObjectID);
}

void FScene::RenderThread_PrepareViewsForRendering()
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
    if (Camera)
    {
        CameraView.SetupFrustum(Camera->Snapshot.View, Camera->Snapshot.Projection);
    }

    // Prepare directional-light shadow-view
    if (DirectionalLight)
    {
        DirectionalLight->ShadowView.PrepareView(StaticMeshes.Capacity());
    }

    // Prepare point-light shadow-views
    for (FScenePointLight* PointLight : PointLights)
    {
        if (!PointLight->bCastShadows)
        {
            continue;
        }

        // Multi-pass (One pass per face)
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            PointLight->ShadowView[FaceIndex].PrepareView();
            PointLight->ShadowView[FaceIndex].SetupFrustum(PointLight->ViewMatrix[FaceIndex], PointLight->ProjMatrix[FaceIndex]);
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
            DirectionalLight->ShadowView.AddStaticMesh(StaticMesh);
        }

        // Update the visibility PointLights
        for (FScenePointLight* PointLight : PointLights)
        {
            if (!PointLight->bCastShadows)
            {
                continue;
            }

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
