#pragma once
#include "Core/Containers/Array.h"
#include "Core/Tasks/TaskHandle.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FSceneRenderer;
class FScene;

class FRendererModule : public IRendererModule
{
public:
    FRendererModule();
    virtual ~FRendererModule();
    
    // IRendererModule Interface 
    virtual bool Load()       override final;
    virtual bool Initialize() override final;
    virtual void Release()    override final;
    virtual void Tick()       override final;

    virtual void FinishPreviousFrame() override final;
    virtual void DiscardPendingFrame() override final;

    virtual void RecordUI() override final;
    
    virtual void KickSceneRender(FSceneRenderPacket&& Packet) override final;
 
    virtual void RequestEditorObjectPick(IScene* Scene, uint32 PixelX, uint32 PixelY) override final; 
    virtual bool PollEditorObjectPickResult(IScene* Scene, uint32& OutObjectID)       override final; 
 
    virtual void ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 Width, uint32 Height, EFormat Format = EFormat::Unknown, EColorSpace ColorSpace = EColorSpace::Unknown) override final; 

    // Creates and adds a scene to the list of scenes
    virtual IScene* CreateScene(FWorld* World) override final;
    
    // Destroys and removes a scene from the list of scenes
    virtual void DestroyScene(IScene* Scene) override final;

    virtual IGPUProfiler& GetGPUProfiler() override final;

    const TArray<FScene*>& GetScenes() const
    {
        return Scenes;
    }

private:
    FSceneRenderer*    Renderer;
    TArray<FScene*>    Scenes;
    FTaskHandle        PendingSceneTask;
    FSceneRenderPacket PendingPacket;
    FDelegateHandle    PreEngineInitHandle; // Delegate that is called to properly initialize ImGui for this module
    bool               bHasPendingFrame;
};
