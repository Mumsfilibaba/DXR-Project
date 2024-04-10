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
    , Camera(nullptr)
    , DirectionalLightIndex(-1)
{
}

FScene::~FScene()
{
    for (FProxySceneComponent* Component : Primitives)
    {
        delete Component;
    }

    Primitives.Clear();
    Lights.Clear();
    LightViews.Clear();

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

    if (LightViews.Size() < Lights.Size())
        LightViews.Resize(Lights.Size());

    // Update LightData
    PointLightViews.Clear();

    for (int32 Index = 0; Index < Lights.Size(); Index++)
    {
        FLightView& LightView = LightViews[Index];
        if (FDirectionalLight* DirectionalLight = Cast<FDirectionalLight>(Lights[Index]))
        {
            // Update LightIndex
            DirectionalLightIndex = Index;

            constexpr int32 NumDirectionalLightViews = 1;
            if (LightView.Primitives.Size() < NumDirectionalLightViews)
            {
                LightView.LightType = FLightView::LightType_Directional;
                LightView.NumSubViews = NumDirectionalLightViews;
                LightView.Frustums.Resize(LightView.NumSubViews);
                LightView.Primitives.Resize(LightView.NumSubViews);
                LightView.MeshBatches.Resize(LightView.NumSubViews);
                LightView.ShadowData.Resize(LightView.NumSubViews);

                LightView.Primitives[0].Reserve(Primitives.Capacity());
            }
        }
        else if (FPointLight* PointLight = Cast<FPointLight>(Lights[Index]))
        {
            if (!PointLight->IsShadowCaster())
            {
                continue;
            }

            constexpr int32 NumPointLightViews = RHI_NUM_CUBE_FACES;
            if (LightView.Primitives.Size() < NumPointLightViews)
            {
                LightView.LightType = FLightView::LightType_Point;
                LightView.NumSubViews = NumPointLightViews;
                LightView.Frustums.Resize(LightView.NumSubViews);
                LightView.Primitives.Resize(LightView.NumSubViews);
                LightView.MeshBatches.Resize(LightView.NumSubViews);
                LightView.ShadowData.Resize(LightView.NumSubViews);
            }

            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
            {
                LightView.Frustums[FaceIndex] = FFrustum(PointLight->GetShadowFarPlane(), PointLight->GetViewMatrix(FaceIndex), PointLight->GetProjectionMatrix(FaceIndex));
                LightView.ShadowData[FaceIndex].Matrix    = PointLight->GetMatrix(FaceIndex);
                LightView.ShadowData[FaceIndex].Position  = PointLight->GetPosition();
                LightView.ShadowData[FaceIndex].NearPlane = PointLight->GetShadowNearPlane();
                LightView.ShadowData[FaceIndex].FarPlane  = PointLight->GetShadowFarPlane();
            }
            
            PointLightViews.Add(&LightView);
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

    for (int32 Index = 0; Index < Lights.Size(); Index++)
    {
        FLightView& LightView = LightViews[Index];
        if (FDirectionalLight* DirectionalLight = Cast<FDirectionalLight>(Lights[Index]))
        {
            LightView.Primitives[0].Clear();
        }
        else if (FPointLight* PointLight = Cast<FPointLight>(Lights[Index]))
        {
            for (FLightView::PrimitivesArray& CubeFacePrimitives : LightView.Primitives)
                CubeFacePrimitives.Clear();
        }
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

        // Update the visibility for lights
        for (int32 Index = 0; Index < Lights.Size(); Index++)
        {
            FLightView& LightView = LightViews[Index];
            if (FDirectionalLight* DirectionalLight = Cast<FDirectionalLight>(Lights[Index]))
            {
                // For now all primitives are visible to the DirectionalLight
                LightView.Primitives[0].Add(Component);
            }
            else if (FPointLight* PointLight = Cast<FPointLight>(Lights[Index]))
            {
                if (PointLight->IsShadowCaster())
                {
                    for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
                    {
                        if (LightView.Frustums[FaceIndex].CheckAABB(Box))
                            LightView.Primitives[FaceIndex].Add(Component);
                    }
                }
            }
        }
    }
}

void FScene::UpdateBatches()
{
    TRACE_SCOPE("UpdateBatches");

    // Clear for this frame
    VisibleMeshBatches.Clear();

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

        FMeshBatch& Batch = VisibleMeshBatches[BatchIndex];
        Batch.Primitives.Add(Component);
    }

    // Batch all lights
    for (FLightView& LightView : LightViews)
    {
        // NOTE: We need to use the Primitives array here due to how DirectionalLights works
        for (int32 SubViewIndex = 0; SubViewIndex < LightView.NumSubViews; SubViewIndex++)
        {
            // Clear map for each LightView
            MaterialToBatchIndex.Clear();

            FLightView::MeshBatchArray& MeshBatchesArray = LightView.MeshBatches[SubViewIndex];
            MeshBatchesArray.Clear();

            for (FProxySceneComponent* Component : LightView.Primitives[SubViewIndex])
            {
                const uint64 MaterialID = reinterpret_cast<uint64>(Component->Material);

                int32 BatchIndex;
                if (int32* ExistingBatchIndex = MaterialToBatchIndex.Find(MaterialID))
                {
                    BatchIndex = *ExistingBatchIndex;
                }
                else
                {
                    const int32 NewIndex = BatchIndex = MeshBatchesArray.Size();
                    MeshBatchesArray.Emplace(Component->Material);
                    MaterialToBatchIndex.Add(MaterialID, NewIndex);
                }

                FMeshBatch& Batch = MeshBatchesArray[BatchIndex];
                Batch.Primitives.Add(Component);
            }
        }
    }
}