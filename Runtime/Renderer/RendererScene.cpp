#include "RendererScene.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Frustum.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

FRendererScene::FRendererScene(FScene* InScene)
    : IRendererScene()
    , Scene(InScene)
    , Primitives()
    , Lights()
    , Camera(nullptr)
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

    Scene  = nullptr;
    Camera = nullptr;
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
    {
        Primitives.Shrink();
    }

    if (VisiblePrimitives.Capacity() < Primitives.Capacity())
    {
        VisiblePrimitives.Reserve(Primitives.Capacity());
    }

    VisiblePrimitives.Clear();

    const FFrustum CameraFrustum = FFrustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
    for (FProxyRendererComponent* Component : Primitives)
    {
        FMatrix4 TransformMatrix = Component->CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        const FVector3 Top    = TransformMatrix.Transform(Component->Mesh->BoundingBox.Top);
        const FVector3 Bottom = TransformMatrix.Transform(Component->Mesh->BoundingBox.Bottom);

        FAABB Box(Top, Bottom);
        if (CameraFrustum.CheckAABB(Box))
        {
            VisiblePrimitives.Add(Component);
        }
    }

    return;
}