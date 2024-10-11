#include "Scene.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Frustum.h"
#include "Engine/World/World.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

bool GFreezeRendering = false;

FMeshBatch::FMeshBatch(FMaterial* InMaterial)
    : Material(InMaterial)
    , Primitives()
{
}

FMeshBatch::~FMeshBatch()
{
}

void FMeshBatch::AddPrimitive(FProxySceneComponent* Primitive, int32 MaterialIndex)
{
    const FSubMesh& SubMesh = Primitive->Mesh->GetSubMesh(MaterialIndex);
    FMeshReference& MeshReference = Primitives.Emplace();
    MeshReference.Primitive    = Primitive;
    MeshReference.SubMeshIndex = MaterialIndex;
    MeshReference.BaseVertex   = SubMesh.BaseVertex;
    MeshReference.StartIndex   = SubMesh.StartIndex;
    MeshReference.VertexCount  = SubMesh.VertexCount;
    MeshReference.IndexCount   = SubMesh.IndexCount;
}

FScene::FScene(FWorld* InWorld)
    : IScene()
    , World(InWorld)
    , Camera(nullptr)
    , Primitives()
    , VisiblePrimitives()
    , VisibleMeshBatches()
    , Lights()
    , DirectionalLight()
    , PointLights()
    , Materials()
{
}

FScene::~FScene()
{
    // Primitives
    for (FProxySceneComponent* Component : Primitives)
    {
        SAFE_DELETE(Component);
    }

    Primitives.Clear();

    // Lights
    for (FScenePointLight* ScenePointLight : PointLights)
    {
        SAFE_DELETE(ScenePointLight);
    }

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
    if (GFreezeRendering)
    {
        return;
    }

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
        
        for (int32 Index = 0; Index < InComponent->Materials.Size(); Index++)
        {
            CHECK(InComponent->Materials[Index] != nullptr);
            Materials.AddUnique(InComponent->Materials[Index].Get());
        }
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
    {
        Primitives.Shrink();
    }

    if (VisiblePrimitives.Capacity() < Primitives.Capacity())
    {
        VisiblePrimitives.Reserve(Primitives.Capacity());
    }

    // Clear for this frame
    VisiblePrimitives.Clear();

    // Clear  DirectionalLight
    if (DirectionalLight)
    {
        DirectionalLight->Primitives.Clear();
    }

    // Clear PointLights
    for (int32 Index = 0; Index < PointLights.Size(); Index++)
    {
        FScenePointLight* ScenePointLight = PointLights[Index];

        // Single Pass
        ScenePointLight->SinglePassPrimitives.Clear();

        // Two Pass
        ScenePointLight->TwoPassPrimitives[0].Clear();
        ScenePointLight->TwoPassPrimitives[1].Clear();

        // Six Pass
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            ScenePointLight->Primitives[FaceIndex].Clear();
        }
    }

    // Perform frustum culling
    const FFrustum CameraFrustum = FFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (FProxySceneComponent* Component : Primitives)
    {
        FMatrix4 TransformMatrix = Component->CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        const FAABB& BoundingBox = Component->Mesh->GetAABB();
        const FVector3 Top    = TransformMatrix.Transform(BoundingBox.Top);
        const FVector3 Bottom = TransformMatrix.Transform(BoundingBox.Bottom);

        // Frustum cull the main view
        FAABB Box(Top, Bottom);
        if (CameraFrustum.CheckAABB(Box))
        {
            Component->UpdateFrustumVisbility(true);
            VisiblePrimitives.Add(Component);
        }
        else
        {
            Component->UpdateFrustumVisbility(false);
        }

        // Update the visibility DirectionalLight
        if (DirectionalLight)
        {
            DirectionalLight->Primitives.Add(Component);
        }

        // Update the visibility PointLights
        for (int32 Index = 0; Index < PointLights.Size(); Index++)
        {
            FScenePointLight* ScenePointLight = PointLights[Index];

            // Single-Pass and 6-Pass
            {
                bool bIsVisibleSinglePass = false;
                for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
                {
                    if (ScenePointLight->Frustums[FaceIndex].CheckAABB(Box))
                    {
                        ScenePointLight->Primitives[FaceIndex].Add(Component);
                        bIsVisibleSinglePass = true;
                    }
                }

                if (bIsVisibleSinglePass)
                {
                    ScenePointLight->SinglePassPrimitives.Add(Component);
                }
            }

            // Two-Pass
            {
                constexpr int32 NumFacesPerPass = 3;

                // First Pass
                bool bIsVisibleTwoPass = false;
                for (int32 FaceIndex = 0; FaceIndex < NumFacesPerPass; FaceIndex++)
                {
                    if (ScenePointLight->Frustums[FaceIndex].CheckAABB(Box))
                    {
                        bIsVisibleTwoPass = true;
                    }
                }

                if (bIsVisibleTwoPass)
                {
                    ScenePointLight->TwoPassPrimitives[0].Add(Component);
                }
                
                // Second Pass
                bIsVisibleTwoPass = false;
                for (int32 FaceIndex = NumFacesPerPass; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
                {
                    if (ScenePointLight->Frustums[FaceIndex].CheckAABB(Box))
                    {
                        bIsVisibleTwoPass = true;
                    }
                }

                if (bIsVisibleTwoPass)
                {
                    ScenePointLight->TwoPassPrimitives[1].Add(Component);
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

    // Batch primitives for the Camera-View
    TMap<uint64, int32> MaterialToBatchIndex;
    for (FProxySceneComponent* Component : VisiblePrimitives)
    {
        const int32 NumMaterials = Component->GetNumMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
        {
            FMaterial* Material = Component->GetMaterial(MaterialIndex);
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

            VisibleMeshBatches[BatchIndex].AddPrimitive(Component, MaterialIndex);
        }
    }

    // Batch primitives for any DirectionalLight
    if (DirectionalLight)
    {
        // Clear map for each LightView
        MaterialToBatchIndex.Clear();
        DirectionalLight->MeshBatches.Clear();

        for (FProxySceneComponent* Component : DirectionalLight->Primitives)
        {
            const int32 NumMaterials = Component->GetNumMaterials();
            for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
            {
                FMaterial* Material = Component->GetMaterial(MaterialIndex);
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
                
                DirectionalLight->MeshBatches[BatchIndex].AddPrimitive(Component, MaterialIndex);
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

        for (FProxySceneComponent* Component : ScenePointLight->SinglePassPrimitives)
        {
            const int32 NumMaterials = Component->GetNumMaterials();
            for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
            {
                FMaterial* Material = Component->GetMaterial(MaterialIndex);
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
                
                ScenePointLight->SinglePassMeshBatch[BatchIndex].AddPrimitive(Component, MaterialIndex);
            }
        }

        // Prepare for two-Pass rendering
        for (int32 PassIndex = 0; PassIndex < 2; PassIndex++)
        {
            MaterialToBatchIndex.Clear();

            TArray<FMeshBatch>& MeshBatches = ScenePointLight->TwoPassMeshBatches[PassIndex];
            MeshBatches.Clear();

            for (FProxySceneComponent* Component : ScenePointLight->TwoPassPrimitives[PassIndex])
            {
                const int32 NumMaterials = Component->GetNumMaterials();
                for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
                {
                    FMaterial* Material = Component->GetMaterial(MaterialIndex);
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
                    
                    MeshBatches[BatchIndex].AddPrimitive(Component, MaterialIndex);
                }
            }
        }

        // Prepare for rendering each face
        for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
        {
            MaterialToBatchIndex.Clear();

            TArray<FMeshBatch>& MeshBatches = ScenePointLight->MeshBatches[FaceIndex];
            MeshBatches.Clear();

            for (FProxySceneComponent* Component : ScenePointLight->Primitives[FaceIndex])
            {
                const int32 NumMaterials = Component->GetNumMaterials();
                for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
                {
                    FMaterial* Material = Component->GetMaterial(MaterialIndex);
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
                    
                    MeshBatches[BatchIndex].AddPrimitive(Component, MaterialIndex);
                }
            }
        }
    }
}
