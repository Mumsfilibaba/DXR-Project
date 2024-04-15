#pragma once
#include "Core/Containers/Array.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FSceneRenderer;
class FScene;

class FRendererModule : public IRendererModule
{
public:
    FRendererModule();
    virtual ~FRendererModule();
    
    virtual bool Load() override final;

    virtual bool Initialize() override final;

    virtual void Tick() override final;

    virtual void Release() override final;

    // Creates and adds a scene to the list of scenes
    virtual IScene* CreateScene(FWorld* World) override final;
    
    // Destroys and removes a scene from the list of scenes
    virtual void DestroyScene(IScene* Scene) override final;

    const TArray<FScene*>& GetScenes() const
    {
        return Scenes;
    }

private:

    // SceneRenderer instance
    FSceneRenderer* Renderer;

    // All scenes that has been allocated and will be rendered
    TArray<FScene*> Scenes;

    // Delegate that is called to properly initialize ImGui for this module
    FDelegateHandle PostApplicationCreateHandle;
};