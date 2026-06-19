#include "Engine/RuntimeEngine.h"
#include "Engine/EngineUI/Runtime/RuntimeConsoleWidget.h"
#include "Engine/EngineUI/Editor/EditorFrameProfilerWidget.h"
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
        ProfilerWidget = MakeSharedPtr<FEditorFrameProfilerWidget>();
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

FSceneRenderPacket FRuntimeEngine::BuildRenderPacket()
{
    TRACE_FUNCTION_SCOPE();

    // Render directly to the BackBuffer.
    FSceneRenderPacket Packet = FEngine::BuildRenderPacket();
    Packet.View.Scene        = GetWorld()->GetSceneInterface();
    Packet.View.RenderTarget = GetSceneViewport()->GetRHISwapChain()->GetBackBuffer();

    return Packet;
}
