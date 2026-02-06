#include "Engine/RuntimeEngine.h"
#include "Engine/EngineUI/Runtime/RuntimeConsoleWidget.h"
#include "Engine/EngineUI/FrameProfilerWidget.h"
#include "RendererCore/Interfaces/IRendererModule.h"

FRuntimeEngine::FRuntimeEngine()
    : FEngine()
    , ConsoleWidget(nullptr)
    , ProfilerWidget(nullptr)
{
}

FRuntimeEngine::~FRuntimeEngine()
{
}

bool FRuntimeEngine::Init()
{
    if (!FEngine::Init())
    {
        return false;
    }

    if (IImguiPlugin::IsEnabled())
    {
        ProfilerWidget = MakeSharedPtr<FFrameProfilerWidget>();
        ConsoleWidget  = MakeSharedPtr<FRuntimeConsoleWidget>();
    }

    // Make sure we have focus on the engine viewport
    if (TSharedPtr<FViewportWidget> Viewport =  FEngine::GetViewportWidget())
    {
        Viewport->SetActivationPolicy(EWidgetActivationPolicy::AutoFocusOnWindowActivate);
        FApplication::Get().SetFocusWidget(FEngine::GetViewportWidget());
    }

    return true;
}

void FRuntimeEngine::Release()
{
    if (IImguiPlugin::IsEnabled())
    {
        ProfilerWidget.Reset();
        ConsoleWidget.Reset();
    }

    FEngine::Release();
}

void FRuntimeEngine::RenderFrame()
{
    TRACE_FUNCTION_SCOPE();

    // Render directly to the BackBuffer
    FSceneRenderView RenderView;
    RenderView.Scene        = GetWorld()->GetSceneInterface();
    RenderView.RenderTarget = GetSceneViewport()->GetRHISwapChain()->GetBackBuffer();

    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->RenderSceneView(RenderView);

    // Render the rest
    FEngine::RenderFrame();
}
