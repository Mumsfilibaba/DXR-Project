#pragma once
#include "IScene.h"
#include "Core/Modules/ModuleManager.h"

class FWorld;

struct IRendererModule : public FModuleInterface
{
    static IRendererModule* Get()
    {
        IRendererModule* RendererModule = FModuleManager::Get().GetModule<IRendererModule>("Renderer");
        return RendererModule;
    }

    virtual ~IRendererModule() = default;

    // Initialize the Renderer from the EngineLoop
    virtual bool Initialize() = 0;

    // Run a frame on the Renderer side
    virtual void Tick() = 0;

    // Release the Renderer from the EngineLoop
    virtual void Release() = 0;

    // Create a Renderer version of the World
    virtual IScene* CreateScene(FWorld* InScene) = 0;

    // Destroy a Renderer Scene
    virtual void DestroyScene(IScene* Scene) = 0;
};
