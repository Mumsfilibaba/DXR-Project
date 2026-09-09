#include "Core/Misc/ConsoleManager.h"
#include "Core/Tasks/Tasks.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/RendererModule.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"

#if EDITOR_BUILD
    #include "RendererCore/Debug/RenderGraphDebug.h"
#endif

IMPLEMENT_ENGINE_MODULE(FRendererModule, Renderer);

FRendererModule::FRendererModule()
    : IRendererModule()
    , Renderer(nullptr)
    , Scenes()
    , bHasPendingFrame(false)
{
    // Self-register the cached module instance returned by IRendererModule::Get().
    CHECK(RendererModule == nullptr);
    RendererModule = this;
}

FRendererModule::~FRendererModule()
{
    for (FScene* Scene : Scenes)
    {
        delete Scene;
    }

    RendererModule = nullptr;
}

bool FRendererModule::Load()
{
    return true;
}

bool FRendererModule::Initialize()
{
    if (Renderer)
    {
        LOG_WARNING("Renderer is already initialized");
        return false;
    }

    Renderer = new FSceneRenderer();
    if (!Renderer->Initialize())
    {
        return false;
    }

    return true;
}

void FRendererModule::Release()
{
    FinishPreviousFrame();

    // Release GPU profiler
    FGPUProfiler::Get().Release();

    if (Renderer)
    {
        delete Renderer;
        Renderer = nullptr;
    }
}

void FRendererModule::Tick()
{
    CHECK_MAIN_THREAD();

    for (FScene* Scene : Scenes)
    {
        Scene->Tick();
    }
}

void FRendererModule::FinishPreviousFrame()
{
    CHECK_MAIN_THREAD();

    if (!bHasPendingFrame)
    {
        return;
    }

    PendingSceneTask.Wait();

    PendingSceneTask = FTaskHandle();
    bHasPendingFrame = false;
}

void FRendererModule::KickSceneRender(FSceneRenderPacket&& Packet)
{
    CHECK_MAIN_THREAD();

    if (!Renderer)
    {
        return;
    }

    bHasPendingFrame = true;

    PendingSceneTask = Tasks::LaunchOnRenderThread("SceneRender",
        [this, Packet = ::Move(Packet)]()
        {
            CHECK_RENDER_THREAD();
            Renderer->RenderThread_RenderSceneFrame(Packet);
        });
}

void FRendererModule::RequestEditorObjectPick(IScene* Scene, uint32 PixelX, uint32 PixelY, uint64 RequestId)
{ 
#if EDITOR_BUILD 
    if (Renderer) 
    { 
        Renderer->RequestEditorObjectPick(static_cast<FScene*>(Scene), PixelX, PixelY, RequestId);
    } 
#else 
    UNREFERENCED_VARIABLE(Scene); 
    UNREFERENCED_VARIABLE(PixelX); 
    UNREFERENCED_VARIABLE(PixelY); 
    UNREFERENCED_VARIABLE(RequestId);
#endif 
} 
 
bool FRendererModule::PollEditorObjectPickResult(IScene* Scene, FEditorPickResult& OutResult)
{ 
#if EDITOR_BUILD 
    if (Renderer) 
    { 
        return Renderer->PollEditorObjectPickResult(static_cast<FScene*>(Scene), OutResult);
    } 
 
    return false; 
#else 
    UNREFERENCED_VARIABLE(Scene); 
    OutResult = FEditorPickResult();
    return false; 
#endif 
} 

void FRendererModule::RequestEditorObjectPickRect(IScene* Scene, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
{
#if EDITOR_BUILD
    if (Renderer)
    {
        Renderer->RequestEditorObjectPickRect(static_cast<FScene*>(Scene), MinX, MinY, MaxX, MaxY);
    }
#else
    UNREFERENCED_VARIABLE(Scene);
    UNREFERENCED_VARIABLE(MinX);
    UNREFERENCED_VARIABLE(MinY);
    UNREFERENCED_VARIABLE(MaxX);
    UNREFERENCED_VARIABLE(MaxY);
#endif
}

bool FRendererModule::PollEditorObjectPickRectResult(IScene* Scene, TArray<uint32>& OutObjectIDs)
{
#if EDITOR_BUILD
    if (Renderer)
    {
        return Renderer->PollEditorObjectPickRectResult(static_cast<FScene*>(Scene), OutObjectIDs);
    }

    return false;
#else
    UNREFERENCED_VARIABLE(Scene);
    OutObjectIDs.Clear();
    return false;
#endif
}

IScene* FRendererModule::CreateScene(FWorld* World)
{
    FScene* NewScene = new FScene(World);
    Scenes.Add(NewScene);
    return NewScene;
}

void FRendererModule::DestroyScene(IScene* Scene)
{
    if (FScene* SceneToRemove = static_cast<FScene*>(Scene))
    {
        Scenes.Remove(SceneToRemove);
        delete SceneToRemove;
    }
}

IGPUProfiler& FRendererModule::GetGPUProfiler()
{
    return FGPUProfiler::Get();
}

#if EDITOR_BUILD
void FRendererModule::SetRenderGraphDebugCaptureEnabled(bool bEnabled)
{
    RenderGraphDebug::SetCaptureEnabled(bEnabled);
}

bool FRendererModule::CopyLatestRenderGraphDebugSnapshot(FRenderGraphDebugSnapshot& Out)
{
    return RenderGraphDebug::CopyLatest(Out);
}
#endif
