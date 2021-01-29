#include "EngineLoop.h"

#include "Engine/EngineGlobals.h"

#include "Time/Clock.h"

#include "Application/Generic/GenericOutputDevice.h"
#include "Application/Generic/GenericCursor.h"
#include "Application/Events/EventDispatcher.h"
#include "Application/Platform/PlatformApplication.h"
#include "Application/Platform/PlatformDialogMisc.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/TextureFactory.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/CommandList.h"

#include "Editor/Editor.h"

#include "Game/Game.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "Memory/Memory.h"

Int32 EngineMain(const TArrayView<const Char*> Args)
{
    UNREFERENCED_VARIABLE(Args);

#ifdef _DEBUG
    Memory::SetDebugFlags(EMemoryDebugFlag::MemoryDebugFlag_LeakCheck);
#endif

    if (!gEngineLoop.PreInit())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Pre-Initialize Failed");
        return -1;
    }

    if (!gEngineLoop.Init())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Initialize Failed");
        return -1;
    }

    if (!gEngineLoop.PostInit())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Post-Initialize Failed");
        return -1;
    }

    while (gEngineLoop.IsRunning())
    {
        TRACE_SCOPE("Tick");

        gEngineLoop.PreTick();

        gEngineLoop.Tick();

        gEngineLoop.PostTick();
    }

    if (!gEngineLoop.PreRelease())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Pre-Release Failed");
        return -1;
    }
    
    if (!gEngineLoop.Release())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Release Failed");
        return -1;
    }
    
    if (!gEngineLoop.PostRelease())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Post-Release Failed");
        return -1;
    }

    return 0;
}

Bool EngineLoop::PreInit()
{
    TRACE_FUNCTION_SCOPE();

    gProfiler.Init();

    gConsoleOutput = PlatformOutputDevice::Make();
    if (!gConsoleOutput)
    {
        return false;
    }
    else
    {
        gConsoleOutput->SetTitle("DXR-Engine Error Output");
    }

    gPlatformApplication = PlatformApplication::Make();
    if (!gPlatformApplication->Init())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Failed to create Platform Application");
        return false;
    }

    return true;
}

Bool EngineLoop::Init()
{
    TRACE_FUNCTION_SCOPE();

    gEventDispatcher = DBG_NEW EventDispatcher(gPlatformApplication);
    gPlatformApplication->SetEventHandler(gEventDispatcher);

    gConsole.Init();

    const UInt32 Style =
        WindowStyleFlag_Titled      |
        WindowStyleFlag_Closable    |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    WindowCreateInfo WinCreateInfo("DXR Engine", 1920, 1080, Style);
    gMainWindow = gPlatformApplication->MakeWindow();
    
    if (!gMainWindow->Init(WinCreateInfo))
    {
        PlatformDialogMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }
    else
    {
        gMainWindow->Show(false);

        INIT_CONSOLE_COMMAND("a.ToggleFullscreen", []() 
        {
            gMainWindow->ToggleFullscreen();
        });

        INIT_CONSOLE_COMMAND("a.Quit", []()
        {
            gEngineLoop.Exit();
        });
    }

    GlobalCursors::Init();

    // RenderAPI
    if (!RenderLayer::Init(ERenderLayerApi::RenderLayerApi_D3D12))
    {
        return false;
    }

    if (!TextureFactory::Init())
    {
        return false;
    }

    if (!gRenderer.Init())
    {
        PlatformDialogMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    if (!DebugUI::Init())
    {
        PlatformDialogMisc::MessageBox("ERROR", "FAILED to create ImGuiContext");
        return false;
    }

    gGame = MakeGameInstance();
    if (!gGame->Init())
    {
        PlatformDialogMisc::MessageBox("ERROR", "FAILED initialize Game");
        return false;
    }

    return true;
}

Bool EngineLoop::PostInit()
{
    TRACE_FUNCTION_SCOPE();

    Editor::Init();

    ShouldRun = true;
    return true;
}

void EngineLoop::PreTick()
{
    TRACE_FUNCTION_SCOPE();

    if (!PlatformApplication::PeekMessageUntilNoMessage())
    {
        Exit();
    }

    gPlatformApplication->Tick();
}

void EngineLoop::Tick()
{
    TRACE_FUNCTION_SCOPE();

    Clock.Tick();

    gGame->Tick(Clock.GetDeltaTime());

    gConsole.Tick();

    Editor::Tick();
}

void EngineLoop::PostTick()
{
    TRACE_FUNCTION_SCOPE();

    gProfiler.Tick();

    gRenderer.Tick(*gGame->GetCurrentScene());
}

Bool EngineLoop::PreRelease()
{
    TRACE_FUNCTION_SCOPE();

    gCmdListExecutor.WaitForGPU();
    
    TextureFactory::Release();

    return true;
}

Bool EngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    SAFEDELETE(gGame);

    DebugUI::Release();

    gRenderer.Release();

    RenderLayer::Release();

    return true;
}

Bool EngineLoop::PostRelease()
{
    TRACE_FUNCTION_SCOPE();

    SAFEDELETE(gEventDispatcher);

    gMainWindow->Release();

    SAFEDELETE(gPlatformApplication);

    SAFEDELETE(gConsoleOutput);

    return true;
}

void EngineLoop::Exit()
{
    ShouldRun = false;
}

Bool EngineLoop::IsRunning() const
{
    return ShouldRun;
}

Timestamp EngineLoop::GetDeltaTime() const
{
    return Clock.GetDeltaTime();
}

Timestamp EngineLoop::GetTotalElapsedTime() const
{
    return Clock.GetTotalTime();
}
