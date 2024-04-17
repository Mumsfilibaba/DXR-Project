#include "Scene.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Frustum.h"
#include "Engine/World/World.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

FScene::FScene(FWorld* InWorld)
    : IScene()
    , World(InWorld)
    , Primitives()
    , Lights()
    , PointLights()
    , Camera(nullptr)
    , DirectionalLight(nullptr)
{
}

FScene::~FScene()
{
    // Primitives
    for (FProxySceneComponent* Component : Primitives)
        SAFE_DELETE(Component);

    Primitives.Clear();

    // Lights
    for (FScenePointLight* ScenePointLight : PointLights)
        SAFE_DELETE(ScenePointLight);
    
    Lights.Clear();
    PointLights.Clear();

    // Remove potential DirectionalLight
    SAFE_DELETE(DirectionalLight);

    // Reset Other stuff
    World  = nullptr;
    Camera = nullptr;
}

void FScene::Tick()
{
    // Updates LightData
    UpdateLights();

    // Performs frustum culling and updates visible primitives
    UpdateVisibility();

    // Batches all the visible primitives based on material
    UpdateBatches();
}

void FScene::AddCamera(FCamera* InCamera)
{
    // TODO: For now it is replacing the current camera
    if (InCamera)
    {
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
            DirectionalLight = new FSceneDirectionalLight(InDirectionalLight);
        }
        else if (FPointLight* InPointLight = Cast<FPointLight>(InLight))
        {
            if (InPointLight->IsShadowCaster())
            {
                FScenePointLight* ScenePointLight = new FScenePointLight(InPointLight); 
                PointLights.Add(ScenePointLight);
            }
        }
    }
}

void FScene::AddProxyComponent(FProxySceneComponent* InComponent)  
{
    if (InComponent)
    {
        Primitives.Add(InComponent);
        Materials.AddUnique(InComponent->Material);
    }
}

void FScene::UpdateLights()
{
    TRACE_SCOPE("UpdateLights");

    // Update DirectionalLight
    if (DirectionalLight)
    {
        if (DirectionalLight->Primitives.Capacity() < Primitives.Capacity())
        {
            DirectionalLight->Primitives.Reserve(Primitives.Capacity());
        }
    }

    // Update PointLights
    for (int32 Index = 0; Index < PointLights.Size(); Index++)
    {
        FScenePointLight* ScenePointLight = PointLights[Index];
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            // Update Frustum
            ScenePointLight->Frustums[FaceIndex] = FFrustum(ScenePointLight->Light->GetShadowFarPlane(), ScenePointLight->Light->GetViewMatrix(FaceIndex), ScenePointLight->Light->GetProjectionMatrix(FaceIndex));
            
            // Update ShadowData
            ScenePointLight->ShadowData[FaceIndex].Matrix    = ScenePointLight->Light->GetMatrix(FaceIndex);
            ScenePointLight->ShadowData[FaceIndex].Position  = ScenePointLight->Light->GetPosition();
            ScenePointLight->ShadowData[FaceIndex].NearPlane = ScenePointLight->Light->GetShadowNearPlane();
            ScenePointLight->ShadowData[FaceIndex].FarPlane  = ScenePointLight->Light->GetShadowFarPlane();
        }
    }
}

void FScene::UpdateVisibility()
{
    TRACE_SCOPE("UpdateVisibility - FrustumCulling");

    if (Primitives.Capacity() > Primitives.Size())
        Primitives.Shrink();

    if (VisiblePrimitives.Capacity() < Primitives.Capacity())
        VisiblePrimitives.Reserve(Primitives.Capacity());

    // Clear for this frame
    VisiblePrimitives.Clear();

    // Clear  DirectionalLight
    if (DirectionalLight)
        DirectionalLight->Primitives.Clear();

    // Clear PointLights
    for (int32 Index = 0; Index < PointLights.Size(); Index++)
    {
        FScenePointLight* ScenePointLight = PointLights[Index];
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            ScenePointLight->Primitives[FaceIndex].Clear();
    }

    // Perform frustum culling
    const FFrustum CameraFrustum = FFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (FProxySceneComponent* Component : Primitives)
    {
        FMatrix4 TransformMatrix = Component->CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        const FVector3 Top    = TransformMatrix.Transform(Component->Mesh->BoundingBox.Top);
        const FVector3 Bottom = TransformMatrix.Transform(Component->Mesh->BoundingBox.Bottom);

        // Frustum cull the main view
        FAABB Box(Top, Bottom);
        if (CameraFrustum.CheckAABB(Box))
        {
            VisiblePrimitives.Add(Component);
        }

        // Update the visibility DirectionalLight
        if (DirectionalLight)
            DirectionalLight->Primitives.Add(Component);

        // Update the visibility PointLights
        for (int32 Index = 0; Index < PointLights.Size(); Index++)
        {
            FScenePointLight* ScenePointLight = PointLights[Index];
            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                if (ScenePointLight->Frustums[FaceIndex].CheckAABB(Box))
                    ScenePointLight->Primitives[FaceIndex].Add(Component);
            }
        }
    }
}

void FScene::UpdateBatches()
{
    TRACE_SCOPE("UpdateBatches");

    // Clear for this frame
    VisibleMeshBatches.Clear();

    // Batch primitives for the Camera-View
    TMap<uint64, int32> MaterialToBatchIndex;
    for (FProxySceneComponent* Component : VisiblePrimitives)
    {
        const uint64 MaterialID = reinterpret_cast<uint64>(Component->Material);

        int32 BatchIndex;
        if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
        {
            BatchIndex = *ExistingBatchIndex;
        }
        else
        {
            const int32 NewIndex = BatchIndex = VisibleMeshBatches.Size();
            VisibleMeshBatches.Emplace(Component->Material);
            MaterialToBatchIndex.Add(MaterialID, NewIndex);
        }

        VisibleMeshBatches[BatchIndex].Primitives.Add(Component);
    }

    // Batch primitives for any DirectionalLight
    if (DirectionalLight)
    {
        // Clear map for each LightView
        MaterialToBatchIndex.Clear();
        DirectionalLight->MeshBatches.Clear();

        for (FProxySceneComponent* Component : DirectionalLight->Primitives)
        {
            const uint64 MaterialID = reinterpret_cast<uint64>(Component->Material);

            int32 BatchIndex;
            if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
            {
                BatchIndex = *ExistingBatchIndex;
            }
            else
            {
                const int32 NewIndex = BatchIndex = DirectionalLight->MeshBatches.Size();
                DirectionalLight->MeshBatches.Emplace(Component->Material);
                MaterialToBatchIndex.Add(MaterialID, NewIndex);
            }

            DirectionalLight->MeshBatches[BatchIndex].Primitives.Add(Component);
        }
    }

    // Batch primitives for any PointLight
    for (int32 Index = 0; Index < PointLights.Size(); Index++)
    {
        FScenePointLight* ScenePointLight = PointLights[Index];
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            // Clear map for each CubeFace
            MaterialToBatchIndex.Clear();

            TArray<FMeshBatch>& MeshBatches = ScenePointLight->MeshBatches[FaceIndex];
            MeshBatches.Clear();

            for (FProxySceneComponent* Component : ScenePointLight->Primitives[FaceIndex])
            {
                const uint64 MaterialID = reinterpret_cast<uint64>(Component->Material);

                int32 BatchIndex;
                if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
                {
                    BatchIndex = *ExistingBatchIndex;
                }
                else
                {
                    const int32 NewIndex = BatchIndex = MeshBatches.Size();
                    MeshBatches.Emplace(Component->Material);
                    MaterialToBatchIndex.Add(MaterialID, NewIndex);
                }

                MeshBatches[BatchIndex].Primitives.Add(Component);
            }
        }
    }
}