#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Frustum.h"
#include "Core/Threading/ScopedLock.h"
#include "Engine/World/World.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/SkyLight.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneSkybox.h"
#include "Renderer/Scene/SceneLightProbe.h"
#include "Renderer/Scene/SceneStaticMesh.h"

bool GFreezeRendering = false;

FScene::FScene(FWorld* InWorld)
    : IScene()
    , World(InWorld)
    , Camera(nullptr)
    , StaticMeshes()
    , VisibleStaticMeshes()
    , VisibleMeshBatches()
    , Lights()
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

    Lights.Clear();
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

    // Reset Other stuff
    World  = nullptr;
    Camera = nullptr;
}

void FScene::Tick()
{
    // Delete all the objects that are deferred
    DeleteDeferredObjects();

    if (GFreezeRendering)
    {
        return;
    }

    // Sync objects with the world
    SyncWithWorld();

    // Updates LightData
    UpdateLights();

    // Performs frustum culling and updates visible primitives
    UpdateVisibility();

    // Prepares primitives for the GPU (Matrices being in correct format etc)
    UpdateStaticMeshes();

    // Batches all the visible primitives based on material
    UpdateBatches();
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
        Lights.Add(InLight);

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

        for (int32 Index = 0; Index < NewStaticMesh->Materials.Size(); Index++)
        {
            CHECK(NewStaticMesh->Materials[Index] != nullptr);
            Materials.AddUnique(NewStaticMesh->Materials[Index].Get());
        }
    }
}

void FScene::SyncWithWorld()
{
    for (FSceneLightProbe* LightProbe : LightProbes)
    {
        LightProbe->Tick();
    }
}

void FScene::UpdateLights()
{
    TRACE_SCOPE("UpdateLights");

    // Update DirectionalLight
    if (DirectionalLight)
    {
        if (DirectionalLight->StaticMeshes.Capacity() < StaticMeshes.Capacity())
        {
            DirectionalLight->StaticMeshes.Reserve(StaticMeshes.Capacity());
        }
    }

    // Update PointLights
    for (int32 Index = 0; Index < PointLights.Size(); Index++)
    {
        FScenePointLight* ScenePointLight = PointLights[Index];
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            // Update Frustum
            ScenePointLight->Frustums[FaceIndex] = FFrustum(ScenePointLight->PointLight->GetShadowFarPlane(), ScenePointLight->PointLight->GetViewMatrix(FaceIndex), ScenePointLight->PointLight->GetProjectionMatrix(FaceIndex));
            
            // Update ShadowData
            FMatrix4 LightMatrix = ScenePointLight->PointLight->GetMatrix(FaceIndex);
            LightMatrix = LightMatrix.GetTranspose();

            ScenePointLight->ShadowData[FaceIndex].Matrix    = LightMatrix;
            ScenePointLight->ShadowData[FaceIndex].Position  = ScenePointLight->PointLight->GetPosition();
            ScenePointLight->ShadowData[FaceIndex].NearPlane = ScenePointLight->PointLight->GetShadowNearPlane();
            ScenePointLight->ShadowData[FaceIndex].FarPlane  = ScenePointLight->PointLight->GetShadowFarPlane();
        }
    }
}

void FScene::UpdateVisibility()
{
    TRACE_SCOPE("UpdateVisibility - FrustumCulling");

    if (StaticMeshes.Capacity() > StaticMeshes.Size())
    {
        StaticMeshes.Shrink();
    }

    if (VisibleStaticMeshes.Capacity() < StaticMeshes.Capacity())
    {
        VisibleStaticMeshes.Reserve(StaticMeshes.Capacity());
    }

    // Clear for this frame
    VisibleStaticMeshes.Clear();

    // Clear  DirectionalLight
    if (DirectionalLight)
    {
        DirectionalLight->StaticMeshes.Clear();
    }

    // Clear PointLights
    for (int32 Index = 0; Index < PointLights.Size(); Index++)
    {
        FScenePointLight* ScenePointLight = PointLights[Index];

        // Single Pass
        ScenePointLight->SinglePassStaticMeshes.Clear();

        // Multi-pass (One pass per face)
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            ScenePointLight->StaticMeshes[FaceIndex].Clear();
        }
    }

    // Perform frustum culling
    const FFrustum CameraFrustum = FFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (FSceneStaticMesh* StaticMesh : StaticMeshes)
    {
        FMatrix4 TransformMatrix = StaticMesh->Actor->GetTransform().GetTransformMatrix();

        const FAABB& BoundingBox = StaticMesh->Mesh->GetAABB();
        const FVector3 Max = TransformMatrix.Transform(BoundingBox.Max);
        const FVector3 Min = TransformMatrix.Transform(BoundingBox.Min);

        // Frustum cull the main view
        FAABB Box(Max, Min);
        if (CameraFrustum.IntersectsAABB(Box))
        {
            StaticMesh->UpdateFrustumVisbility(true);
            VisibleStaticMeshes.Add(StaticMesh);
        }
        else
        {
            StaticMesh->UpdateFrustumVisbility(false);
        }

        // Update the visibility DirectionalLight
        if (DirectionalLight)
        {
            DirectionalLight->StaticMeshes.Add(StaticMesh);
        }

        // Update the visibility PointLights
        for (int32 Index = 0; Index < PointLights.Size(); Index++)
        {
            FScenePointLight* ScenePointLight = PointLights[Index];

            // Check if for each face if a primitive is visible...
            bool bIsVisibleSinglePass = false;
            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                if (ScenePointLight->Frustums[FaceIndex].IntersectsAABB(Box))
                {
                    ScenePointLight->StaticMeshes[FaceIndex].Add(StaticMesh);
                    bIsVisibleSinglePass = true;
                }
            }

            // ... if it is visible for any face we add it to the single-pass
            if (bIsVisibleSinglePass)
            {
                ScenePointLight->SinglePassStaticMeshes.Add(StaticMesh);
            }
        }
    }
}

void FScene::UpdateStaticMeshes()
{
    TRACE_SCOPE("UpdateStaticMeshes");

    for (FSceneStaticMesh* StaticMesh : StaticMeshes)
    {
        StaticMesh->Tick();
    }
}

void FScene::UpdateBatches()
{
    TRACE_SCOPE("UpdateBatches");

    // Clear for this frame
    VisibleMeshBatches.Clear();

    // Batch primitives for the Camera-View
    TMap<uint64, int32> MaterialToBatchIndex;
    for (FSceneStaticMesh* StaticMesh : VisibleStaticMeshes)
    {
        const int32 NumMaterials = StaticMesh->GetNumMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
        {
            FMaterial* Material = StaticMesh->GetMaterial(MaterialIndex);
            const uint64 MaterialID = reinterpret_cast<uint64>(Material);

            int32 BatchIndex;
            if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
            {
                BatchIndex = *ExistingBatchIndex;
            }
            else
            {
                BatchIndex = VisibleMeshBatches.Size();
                VisibleMeshBatches.Emplace(Material);
                MaterialToBatchIndex.Add(MaterialID, BatchIndex);
            }

            VisibleMeshBatches[BatchIndex].AddStaticMesh(StaticMesh, MaterialIndex);
        }
    }

    // Batch primitives for any DirectionalLight
    if (DirectionalLight)
    {
        // Clear map for each LightView
        MaterialToBatchIndex.Clear();
        DirectionalLight->MeshBatches.Clear();

        for (FSceneStaticMesh* StaticMesh : DirectionalLight->StaticMeshes)
        {
            const int32 NumMaterials = StaticMesh->GetNumMaterials();
            for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
            {
                FMaterial* Material = StaticMesh->GetMaterial(MaterialIndex);
                const uint64 MaterialID = reinterpret_cast<uint64>(Material);
                
                int32 BatchIndex;
                if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
                {
                    BatchIndex = *ExistingBatchIndex;
                }
                else
                {
                    BatchIndex = DirectionalLight->MeshBatches.Size();
                    DirectionalLight->MeshBatches.Emplace(Material);
                    MaterialToBatchIndex.Add(MaterialID, BatchIndex);
                }
                
                DirectionalLight->MeshBatches[BatchIndex].AddStaticMesh(StaticMesh, MaterialIndex);
            }
        }
    }

    // Batch primitives for any PointLight
    for (int32 Index = 0; Index < PointLights.Size(); Index++)
    {
        FScenePointLight* ScenePointLight = PointLights[Index];

        // Prepare for single-Pass rendering
        MaterialToBatchIndex.Clear();
        ScenePointLight->SinglePassMeshBatch.Clear();

        for (FSceneStaticMesh* StaticMesh : ScenePointLight->SinglePassStaticMeshes)
        {
            const int32 NumMaterials = StaticMesh->GetNumMaterials();
            for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
            {
                FMaterial* Material = StaticMesh->GetMaterial(MaterialIndex);
                const uint64 MaterialID = reinterpret_cast<uint64>(Material);
                
                int32 BatchIndex;
                if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
                {
                    BatchIndex = *ExistingBatchIndex;
                }
                else
                {
                    BatchIndex = ScenePointLight->SinglePassMeshBatch.Size();
                    ScenePointLight->SinglePassMeshBatch.Emplace(Material);
                    MaterialToBatchIndex.Add(MaterialID, BatchIndex);
                }
                
                ScenePointLight->SinglePassMeshBatch[BatchIndex].AddStaticMesh(StaticMesh, MaterialIndex);
            }
        }

        // Prepare for rendering each face
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            MaterialToBatchIndex.Clear();

            TArray<FMeshBatch>& MeshBatches = ScenePointLight->MeshBatches[FaceIndex];
            MeshBatches.Clear();

            for (FSceneStaticMesh* StaticMesh : ScenePointLight->StaticMeshes[FaceIndex])
            {
                const int32 NumMaterials = StaticMesh->GetNumMaterials();
                for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
                {
                    FMaterial* Material = StaticMesh->GetMaterial(MaterialIndex);
                    const uint64 MaterialID = reinterpret_cast<uint64>(Material);
                    
                    int32 BatchIndex;
                    if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
                    {
                        BatchIndex = *ExistingBatchIndex;
                    }
                    else
                    {
                        BatchIndex = MeshBatches.Size();
                        MeshBatches.Emplace(Material);
                        MaterialToBatchIndex.Add(MaterialID, BatchIndex);
                    }
                    
                    MeshBatches[BatchIndex].AddStaticMesh(StaticMesh, MaterialIndex);
                }
            }
        }
    }
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