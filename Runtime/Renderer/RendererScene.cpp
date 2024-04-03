#include "RendererScene.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Frustum.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

FRendererScene::FRendererScene(FScene* InScene)
    : IRendererScene()
    , Scene(InScene)
    , Primitives()
    , Lights()
    , Camera(nullptr)
    , DirectionalLightIndex(-1)
{
}

FRendererScene::~FRendererScene()
{
    for (FProxyRendererComponent* Component : Primitives)
    {
        delete Component;
    }

    Primitives.Clear();
    Lights.Clear();
    LightViews.Clear();

    Scene  = nullptr;
    Camera = nullptr;
}

void FRendererScene::Tick()
{
    // Performs frustum culling and updates visible primitives
    UpdateVisibility();

    // Batches all the visible primitives based on material
    UpdateBatches();
}

void FRendererScene::AddCamera(FCamera* InCamera)
{
    // TODO: For now it is replacing the current camera
    if (InCamera)
    {
        Camera = InCamera;
    }
}

void FRendererScene::AddLight(FLight* InLight)
{
    if (InLight)
    {
        Lights.Add(InLight);
    }
}

void FRendererScene::AddProxyComponent(FProxyRendererComponent* InComponent)  
{
    if (InComponent)
    {
        Primitives.Add(InComponent);
        Materials.AddUnique(InComponent->Material);
    }
}

void FRendererScene::UpdateVisibility()
{
    TRACE_SCOPE("UpdateVisibility - FrustumCulling");

    if (Primitives.Capacity() > Primitives.Size())
        Primitives.Shrink();

    if (VisiblePrimitives.Capacity() < Primitives.Capacity())
        VisiblePrimitives.Reserve(Primitives.Capacity());

    if (LightViews.Size() < Lights.Size())
        LightViews.Resize(Lights.Size());

    // Clear for this frame
    VisiblePrimitives.Clear();

    for (int32 Index = 0; Index < Lights.Size(); Index++)
    {
        FLight*     Light     = Lights[Index];
        FLightView& LightView = LightViews[Index];

        if (FDirectionalLight* DirectionalLight = Cast<FDirectionalLight>(Light))
        {
            constexpr int32 NumDirectionalLightViews = 1;
            if (LightView.Primitives.Size() < NumDirectionalLightViews)
            {
                LightView.LightType   = FLightView::LightType_Directional;
                LightView.NumSubViews = NumDirectionalLightViews;
                LightView.Frustums.Resize(LightView.NumSubViews);
                LightView.Primitives.Resize(LightView.NumSubViews);
                LightView.MeshBatches.Resize(LightView.NumSubViews);
                LightView.Primitives[0].Reserve(Primitives.Capacity());
            }

            LightView.Primitives[0].Clear();
        }
        else if (FPointLight* PointLight = Cast<FPointLight>(Light))
        {
            constexpr int32 NumPointLightViews = RHI_NUM_CUBE_FACES;
            if (LightView.Primitives.Size() < NumPointLightViews)
            {
                LightView.LightType   = FLightView::LightType_Point;
                LightView.NumSubViews = NumPointLightViews;
                LightView.Frustums.Resize(LightView.NumSubViews);
                LightView.Primitives.Resize(LightView.NumSubViews);
                LightView.MeshBatches.Resize(LightView.NumSubViews);
            }

            for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
                LightView.Frustums[FaceIndex] = FFrustum(PointLight->GetShadowFarPlane(), PointLight->GetViewMatrix(FaceIndex), PointLight->GetProjectionMatrix(FaceIndex));

            for (FLightView::PrimitivesArray& CubeFacePrimitives : LightView.Primitives)
            {
                CubeFacePrimitives.Clear();
            }
        }
    }

    // Perform frustum culling
    const FFrustum CameraFrustum = FFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (FProxyRendererComponent* Component : Primitives)
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
            FLight*     Light     = Lights[Index];
            FLightView& LightView = LightViews[Index];

            if (FDirectionalLight* DirectionalLight = Cast<FDirectionalLight>(Light))
            {
                // For now all primitives are visible to the DirectionalLight
                LightView.Primitives[0].Add(Component);
                DirectionalLightIndex = Index;
            }
            else if (FPointLight* PointLight = Cast<FPointLight>(Light))
            {
                for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
                {
                    if (LightView.Frustums[FaceIndex].CheckAABB(Box))
                    {
                        LightView.Primitives[FaceIndex].Add(Component);
                    }
                }
            }
        }
    }
}

void FRendererScene::UpdateBatches()
{
    TRACE_SCOPE("UpdateBatches");

    // Clear for this frame
    VisibleMeshBatches.Clear();

    TMap<uint64, int32> MaterialToBatchIndex;
    for (FProxyRendererComponent* Component : VisiblePrimitives)
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

            for (FProxyRendererComponent* Component : LightView.Primitives[SubViewIndex])
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