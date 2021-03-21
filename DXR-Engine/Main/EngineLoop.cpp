#include "EngineLoop.h"

#include "Engine/EngineGlobals.h"

#include "Core/Application/Application.h"
#include "Core/Application/Generic/GenericOutputDevice.h"
#include "Core/Application/Generic/GenericCursor.h"
#include "Core/Application/Platform/PlatformApplication.h"
#include "Core/Application/Platform/PlatformMisc.h"
#include "Core/Application/InputManager.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/Resources/TextureFactory.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/CommandList.h"

#include "Editor/Editor.h"

#include "Debug/Profiler.h"
#include "Debug/Console/Console.h"

#include "Memory/Memory.h"

EngineLoop GEngineLoop;

bool EngineLoop::PreInit()
{
    TRACE_FUNCTION_SCOPE();

    Profiler::Init();

    GConsoleOutput = PlatformOutputDevice::Create();
    if (!GConsoleOutput)
    {
        return false;
    }
    else
    {
        GConsoleOutput->SetTitle("DXR-Engine Error Output");
    }

    if (!PlatformApplication::Get().Init())
    {
        PlatformMisc::MessageBox("ERROR", "Failed to create Platform Application");
        return false;
    }

    return true;
}

bool EngineLoop::Init()
{
    TRACE_FUNCTION_SCOPE();

    if (!PreInit())
    {
        return false;
    }

    // RenderAPI
    if (!RenderLayer::Init(ERenderLayerApi::D3D12))
    {
        return false;
    }

    if (!TextureFactory::Init())
    {
        return false;
    }

    // Init Application
    GApplication = CreateApplication();
    Assert(GApplication != nullptr);

    PlatformApplication::Get().SetEventHandler(GApplication);

    if (!GApplication->Init())
    {
        return false;
    }

    if (!InputManager::Get().Init())
    {
        return false;
    }

    if (!GRenderer.Init())
    {
        PlatformMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    GConsole.Init();

    if (!DebugUI::Init())
    {
        PlatformMisc::MessageBox("ERROR", "FAILED to create ImGuiContext");
        return false;
    }

    if (!PostInit())
    {
        return false;
    }

    return true;
}

bool EngineLoop::PostInit()
{
    TRACE_FUNCTION_SCOPE();

    Editor::Init();
    return true;
}

void EngineLoop::Tick()
{
    TRACE_FUNCTION_SCOPE();

    PlatformApplication::Get().Tick();

    Clock.Tick();

    GApplication->Tick(Clock.GetDeltaTime());

    GConsole.Tick();

    Editor::Tick();

    Profiler::Tick();

    GRenderer.Tick(*GApplication->Scene);
}

bool EngineLoop::PreRelease()
{
    TRACE_FUNCTION_SCOPE();

    GCmdListExecutor.WaitForGPU();
    
    TextureFactory::Release();

    return true;
}

bool EngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    if (!PreRelease())
    {
        return false;
    }

    if (GApplication->Release())
    {
        SafeDelete(GApplication);
        PlatformApplication::Get().SetEventHandler(nullptr);
    }
    else
    {
        return false;
    }

    DebugUI::Release();

    GRenderer.Release();

    RenderLayer::Release();

    if (!PostRelease())
    {
        return false;
    }

    return true;
}

bool EngineLoop::PostRelease()
{
    TRACE_FUNCTION_SCOPE();

    PlatformApplication::Get().Release();

    SafeDelete(GConsoleOutput);

    return true;
}

Timestamp EngineLoop::GetDeltaTime()
{
    return Clock.GetDeltaTime();
}

Timestamp EngineLoop::GetRunningTime()
{
    return Clock.GetTotalTime();
}
