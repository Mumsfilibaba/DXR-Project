#include "Engine/RuntimeEngine.h"
#include "Engine/EngineUI/Runtime/OverlayConsole.h"
#include "Engine/EngineUI/Runtime/RuntimeConsoleWidget.h"
#include "Engine/EngineUI/Editor/EditorFrameProfilerWidget.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Core/Misc/ConsoleManager.h"
#include "ImGuiPlugin/ImGuiCore.h"
#include "RendererCore/Interfaces/IRendererModule.h"

static TAutoConsoleVariable<bool> CVarUseOverlayConsole(
    "Engine.UseOverlayConsole",
    "True draws the console with the Application element library, false falls back to the ImGui console",
    true,
    EConsoleVariableFlags::Default);

FRuntimeEngine::FRuntimeEngine()
    : FEngine()
    , LastRenderCamera(nullptr)
    , OverlayConsole(nullptr)
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

    const bool bUseOverlayConsole = CVarUseOverlayConsole.GetValue();
    if (bUseOverlayConsole)
    {
        OverlayConsole = MakeSharedPtr<FOverlayConsole>();
        OverlayConsole->Initialize();
    }

    if (IImguiPlugin::IsEnabled())
    {
        ProfilerWidget = MakeSharedPtr<FEditorFrameProfilerWidget>();

        if (!bUseOverlayConsole)
        {
            ConsoleWidget = MakeSharedPtr<FRuntimeConsoleWidget>();
        }
    }

    // Make sure we have focus on the engine viewport
    if (TSharedPtr<FViewport> Viewport =  FEngine::GetViewport())
    {
        Viewport->SetActivationPolicy(EElementActivationPolicy::AutoFocusOnWindowActivate);
        FApplication::Get().SetFocusElement(FEngine::GetViewport());
    }

    return true;
}

bool FRuntimeEngine::Start()
{
    if (!FEngine::Start())
    {
        return false;
    }

    return StartPlay();
}

void FRuntimeEngine::Release()
{
    StopPlay();

    if (OverlayConsole)
    {
        OverlayConsole->Release();
        OverlayConsole.Reset();
    }

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

    FSceneRenderPacket Packet = FEngine::BuildRenderPacket();
    Packet.View.Scene        = GetWorld()->GetSceneInterface();
    Packet.View.RenderTarget = GetViewportImage();

    if (FCameraComponent* Camera = GetWorld()->GetActiveCamera())
    {
        Camera->PrepareSceneViewInfo(Packet.View.CameraSnapshot);
        
        Packet.View.bHasCamera = true;
        Packet.View.bCameraCut = Camera != LastRenderCamera;

        LastRenderCamera = Camera;
    }
    else
    {
        LastRenderCamera = nullptr;
    }

    return Packet;
}
