#pragma once
#include "IRendererScene.h"
#include "Core/Modules/ModuleManager.h"

class FScene;

struct IRendererModule : public FModuleInterface
{
    static IRendererModule* Get()
    {
        return FModuleManager::Get().GetModule<IRendererModule>("Renderer");
    }

    virtual ~IRendererModule() = default;

    // Initialize the Renderer from the EngineLoop
    virtual bool Initialize() = 0;

    // Run a frame on the Renderer side
    virtual void Tick() = 0;

    // Release the Renderer from the EngineLoop
    virtual void Release() = 0;

    // Create a Renderer version of the Scene
    virtual IRendererScene* CreateRendererScene(FScene* InScene) = 0;

    // Destroy a Renderer Scene
    virtual void DestroyRendererScene(IRendererScene* Scene) = 0;
};