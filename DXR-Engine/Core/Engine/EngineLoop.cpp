#include "Engine.h"
#include "EngineLoop.h"

#include "Core/Engine/EngineGlobals.h"
#include "Core/Application/Application.h"
#include "Core/Application/Generic/OutputConsole.h"
#include "Core/Application/Platform/Platform.h"
#include "Core/Application/Platform/PlatformMisc.h"
#include "Core/Input/InputManager.h"
#include "Core/Threading/Generic/Thread.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/Resources/TextureFactory.h"

#include "Editor/Editor.h"

#include "Debug/Profiler.h"
#include "Debug/Console/Console.h"

#include "Memory/Memory.h"

bool EngineLoop::Init()
{
    TRACE_FUNCTION_SCOPE();

    GConsoleOutput = OutputConsole::Create();
    if (!GConsoleOutput)
    {
        return false;
    }
    else
    {
        GConsoleOutput->SetTitle("DXR-Engine Error Output");
    }

    Profiler::Init();

    if (!Platform::Init())
    {
        PlatformMisc::MessageBox("ERROR", "Failed to create Platform Application");
        return false;
    }

    if (!GEngine.Init())
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

    Platform::SetCallbacks(&GEngine);

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

    Editor::Init();

    return true;
}

void EngineLoop::Tick(Timestamp Deltatime)
{
    TRACE_FUNCTION_SCOPE();

    Platform::Tick();

    GApplication->Tick(Deltatime);

    GConsole.Tick();

    Editor::Tick();

    Profiler::Tick();

    GRenderer.Tick(*GApplication->Scene);
}

void EngineLoop::Run()
{
    Timer Timer;

    while (GEngine.IsRunning)
    {
        Timer.Tick();
        EngineLoop::Tick(Timer.GetDeltaTime());
    }
}

bool EngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    GCmdListExecutor.WaitForGPU();

    TextureFactory::Release();

    if (GApplication->Release())
    {
        SafeDelete(GApplication);
        Platform::SetCallbacks(nullptr);
    }
    else
    {
        return false;
    }

    DebugUI::Release();

    GRenderer.Release();

    RenderLayer::Release();

    Platform::Release();

    SafeDelete(GConsoleOutput);

    return true;
}
