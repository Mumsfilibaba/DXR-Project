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
#include "Rendering/Resources/TextureFactory.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/CommandList.h"

#include "Editor/Editor.h"

#include "Game/Game.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "Memory/Memory.h"

struct EngineLoopData
{
    Bool  ShouldRun = false;
    Bool  IsExiting = false;
    Clock Clock;
};

static EngineLoopData gEngineLoopData;

Int32 EngineMain(const TArrayView<const Char*> Args)
{
    UNREFERENCED_VARIABLE(Args);

#ifdef _DEBUG
    Memory::SetDebugFlags(EMemoryDebugFlag::MemoryDebugFlag_LeakCheck);
#endif

    if (!EngineLoop::PreInit())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Pre-Initialize Failed");
        return -1;
    }

    if (!EngineLoop::Init())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Initialize Failed");
        return -1;
    }

    if (!EngineLoop::PostInit())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Post-Initialize Failed");
        return -1;
    }

    while (EngineLoop::IsRunning())
    {
        TRACE_SCOPE("Tick");

        EngineLoop::PreTick();

        EngineLoop::Tick();

        EngineLoop::PostTick();
    }

    if (!EngineLoop::PreRelease())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Pre-Release Failed");
        return -1;
    }
    
    if (!EngineLoop::Release())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Release Failed");
        return -1;
    }
    
    if (!EngineLoop::PostRelease())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Post-Release Failed");
        return -1;
    }

    return 0;
}

Bool EngineLoop::PreInit()
{
    TRACE_FUNCTION_SCOPE();

    Profiler::Init();

    gConsoleOutput = PlatformOutputDevice::Make();
    if (!gConsoleOutput)
    {
        return false;
    }
    else
    {
        gConsoleOutput->SetTitle("DXR-Engine Error Output");
    }

    gApplication = PlatformApplication::Make();
    if (!gApplication->Init())
    {
        PlatformDialogMisc::MessageBox("ERROR", "Failed to create Platform Application");
        return false;
    }

    return true;
}

Bool EngineLoop::Init()
{
    TRACE_FUNCTION_SCOPE();

    gEventDispatcher = DBG_NEW EventDispatcher(gApplication);
    gApplication->SetEventHandler(gEventDispatcher);

    gConsole.Init();

    const UInt32 Style =
        WindowStyleFlag_Titled      |
        WindowStyleFlag_Closable    |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    WindowCreateInfo WinCreateInfo("DXR Engine", 1920, 1080, Style);
    gMainWindow = gApplication->MakeWindow();
    
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
            EngineLoop::Exit();
        });
    }

    GlobalCursors::Init();

    // RenderAPI
    if (!RenderLayer::Init(ERenderLayerApi::D3D12))
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

    gEngineLoopData.ShouldRun = true;
    return true;
}

void EngineLoop::PreTick()
{
    TRACE_FUNCTION_SCOPE();

    if (!PlatformApplication::PeekMessageUntilNoMessage())
    {
        Exit();
    }

    gApplication->Tick();
}

void EngineLoop::Tick()
{
    TRACE_FUNCTION_SCOPE();

    gEngineLoopData.Clock.Tick();

    gGame->Tick(gEngineLoopData.Clock.GetDeltaTime());

    gConsole.Tick();

    Editor::Tick();
}

void EngineLoop::PostTick()
{
    TRACE_FUNCTION_SCOPE();

    Profiler::Tick();

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

    SafeDelete(gGame);

    DebugUI::Release();

    gRenderer.Release();

    RenderLayer::Release();

    return true;
}

Bool EngineLoop::PostRelease()
{
    TRACE_FUNCTION_SCOPE();

    SafeDelete(gEventDispatcher);

    gMainWindow->Release();

    SafeDelete(gApplication);

    SafeDelete(gConsoleOutput);

    return true;
}

void EngineLoop::Exit()
{
    gEngineLoopData.ShouldRun = false;
    gEngineLoopData.IsExiting = true;
}

Bool EngineLoop::IsRunning()
{
    return gEngineLoopData.ShouldRun;
}

Bool EngineLoop::IsExiting()
{
    return gEngineLoopData.IsExiting;
}

Timestamp EngineLoop::GetDeltaTime()
{
    return gEngineLoopData.Clock.GetDeltaTime();
}

Timestamp EngineLoop::GetTotalElapsedTime()
{
    return gEngineLoopData.Clock.GetTotalTime();
}
