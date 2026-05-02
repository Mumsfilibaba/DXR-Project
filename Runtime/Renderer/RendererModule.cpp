#include "Core/Misc/CoreDelegates.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/RendererModule.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"

IMPLEMENT_ENGINE_MODULE(FRendererModule, Renderer);

FRendererModule::FRendererModule()
    : IRendererModule()
    , Renderer(nullptr)
    , Scenes()
{
}

FRendererModule::~FRendererModule()
{
    CoreDelegates::PreEngineInitDelegate.Unbind(PreEngineInitHandle);

    for (FScene* Scene : Scenes)
    {
        delete Scene;
    }
}

bool FRendererModule::Load()
{
    PreEngineInitHandle = CoreDelegates::PreEngineInitDelegate.AddLambda([this]()
    {
        if (IImguiPlugin::IsEnabled())
        {
            ImGuiContext* Context = IImguiPlugin::Get().GetImGuiContext();
            ImGui::SetCurrentContext(Context);
        }
        else
        {
            CHECK(false);
        }
    });

    return true;
}

bool FRendererModule::Initialize()
{
    if (!Renderer)
    {
        Renderer = new FSceneRenderer();
        return Renderer->Initialize();
    }
    else
    {
        LOG_WARNING("Renderer is already initialized");
        return false;
    }
}

void FRendererModule::Release()
{
    // Release GPU profiler
    FGPUProfiler::Get().Release();

    if (Renderer)
    {
        delete Renderer;
        Renderer = nullptr;
    }
}

void FRendererModule::BeginFrame()
{
    if (Renderer)
    {
        Renderer->BeginFrame();
    }
}

void FRendererModule::Tick()
{
    for (FScene* Scene : Scenes)
    {
        // Performs frustum culling for all the cameras and updates visible primitives
        Scene->Tick();
    }
}

void FRendererModule::EndFrame()
{
    if (Renderer)
    {
        Renderer->EndFrame();
    }
}

void FRendererModule::RenderSceneView(const FSceneRenderView& SceneRenderView)
{
    if (Renderer)
    {
        Renderer->RenderSceneView(SceneRenderView);
    }
}

void FRendererModule::RenderUI()
{
    if (Renderer)
    {
        Renderer->RenderUI();
    }
}

void FRendererModule::RequestEditorObjectPick(IScene* Scene, uint32 PixelX, uint32 PixelY) 
{ 
#if EDITOR_BUILD 
    if (Renderer) 
    { 
        Renderer->RequestEditorObjectPick(static_cast<FScene*>(Scene), PixelX, PixelY); 
    } 
#else 
    UNREFERENCED_VARIABLE(Scene); 
    UNREFERENCED_VARIABLE(PixelX); 
    UNREFERENCED_VARIABLE(PixelY); 
#endif 
} 
 
bool FRendererModule::PollEditorObjectPickResult(IScene* Scene, uint32& OutObjectID) 
{ 
#if EDITOR_BUILD 
    if (Renderer) 
    { 
        return Renderer->PollEditorObjectPickResult(static_cast<FScene*>(Scene), OutObjectID); 
    } 
 
    return false; 
#else 
    UNREFERENCED_VARIABLE(Scene); 
    OutObjectID = 0; 
    return false; 
#endif 
} 
 
void FRendererModule::PrepareSwapChain(FRHISwapChainRef SwapChain) 
{ 
    if (Renderer)
    {
        Renderer->PrepareSwapChain(SwapChain);
    }
}

void FRendererModule::PresentSwapChain(FRHISwapChainRef SwapChain)
{
    if (Renderer)
    {
        Renderer->PresentSwapChain(SwapChain);
    }
}

void FRendererModule::ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    if (Renderer)
    {
        Renderer->ResizeSwapChain(SwapChain, Width, Height, Format, ColorSpace);
    }
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
