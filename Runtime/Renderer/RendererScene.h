#pragma once
#include "Core/Containers/Array.h"
#include "RendererCore/Interfaces/IRendererScene.h"

class FScene;

class FRendererScene : public IRendererScene
{
public:
    FRendererScene(FScene* InScene);
    virtual ~FRendererScene();

    // Adds a camera to the scene
    virtual void AddCamera(FCamera* InCamera) override final;

    // Adds a light to the scene
    virtual void AddLight(FLight* InLight) override final;

    // TODO: Adds a new mesh to be drawn, but most renderer primitives should take this path
    virtual void AddProxyComponent(FProxyRendererComponent* InComponent) override final;

    // Scene that is mirrored by this RendererScene
    FScene* Scene;

    // TODO: Differ the Renderer's camera from the Scene's
    FCamera* Camera;

    // All primitives in this scene
    TArray<FProxyRendererComponent*> Primitives;

    // All Lights in the Scene
    TArray<FLight*> Lights;
};