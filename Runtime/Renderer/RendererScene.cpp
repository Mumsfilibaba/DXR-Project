#include "RendererScene.h"
#include "Engine/Scene/Scene.h"

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
    }
}