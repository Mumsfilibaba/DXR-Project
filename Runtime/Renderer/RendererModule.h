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

    virtual void KickSceneRender(FSceneRenderPacket&& Packet) override final;

    virtual void RequestEditorObjectPick(IScene* Scene, uint32 PixelX, uint32 PixelY, uint64 RequestId) override final;
    virtual bool PollEditorObjectPickResult(IScene* Scene, FEditorPickResult& OutResult)                override final;

    virtual void RequestEditorObjectPickRect(IScene* Scene, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY) override final;
    virtual bool PollEditorObjectPickRectResult(IScene* Scene, TArray<uint32>& OutObjectIDs)                    override final;

    // Creates and adds a scene to the list of scenes
    virtual IScene* CreateScene(FWorld* World) override final;
    
    // Destroys and removes a scene from the list of scenes
    virtual void DestroyScene(IScene* Scene) override final;

    virtual IGPUProfiler& GetGPUProfiler() override final;

#if EDITOR_BUILD
    virtual void SetRenderGraphDebugCaptureEnabled(bool bEnabled) override final;
    virtual bool CopyLatestRenderGraphDebugSnapshot(FRenderGraphDebugSnapshot& Out) override final;
#endif

    const TArray<FScene*>& GetScenes() const
    {
        return Scenes;
    }

private:
    FSceneRenderer* Renderer;
    TArray<FScene*> Scenes;
    FTaskHandle     PendingSceneTask;
    bool            bHasPendingFrame;
};
