#pragma once
#include "Core/Containers/Array.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FSceneRenderer;
class FRendererScene;

class FRendererModule : public IRendererModule
{
public:
    FRendererModule();
    virtual ~FRendererModule();
    
    virtual bool Load() override final;

    // Initialize the Renderer from the EngineLoop
    virtual bool Initialize() override final;

    // Run a frame on the Renderer side
    virtual void Tick() override final;

    // Release the Renderer from the EngineLoop
    virtual void Release() override final;

    // Creates and adds a scene to the list of scenes
    virtual IRendererScene* CreateRendererScene(FScene* Scene) override final;
    
    // Destroys and removes a scene from the list of scenes
    virtual void DestroyRendererScene(IRendererScene* Scene) override final;

    const TArray<FRendererScene*>& GetScenes() const
    {
        return Scenes;
    }

private:

    // SceneRenderer instance
    FSceneRenderer* Renderer;

    // All scenes that has been allocated and will be rendered
    TArray<FRendererScene*> Scenes;

    // Delegate that is called to properly initialize ImGui for this module
    FDelegateHandle PostApplicationCreateHandle;
};