#include "Engine.h"
#include "EngineLoop.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/Resources/TextureFactory.h"

#include "Editor/Editor.h"

#include "Core/Memory/Memory.h"
#include "Core/Engine/EngineGlobals.h"
#include "Core/Application/Application.h"
#include "Core/Application/Generic/GenericOutputConsole.h"
#include "Core/Application/Platform/Platform.h"
#include "Core/Application/Platform/PlatformMisc.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Input/InputManager.h"
#include "Core/Debug/Profiler.h"
#include "Core/Debug/Console/Console.h"
#include "Core/Threading/TaskManager.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/InterlockedInt.h"
#include "Core/Threading/Platform/PlatformProcess.h"

bool EngineLoop::Init()
{
    TRACE_FUNCTION_SCOPE();

    GConsoleOutput = PlatformMisc::CreateOutputConsole();
    if ( !GConsoleOutput )
    {
        return false;
    }
    else
    {
        GConsoleOutput->SetTitle( "DXR-Engine Error Console" );
    }

    Profiler::Init();

    if ( !Platform::Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to init Platform" );
        return false;
    }

    if ( !TaskManager::Get().Init() )
    {
        return false;
    }

    if ( !GEngine.Init() )
    {
        return false;
    }

    // RenderAPI
    if ( !RenderLayer::Init( ERenderLayerApi::D3D12 ) )
    {
        return false;
    }

    if ( !TextureFactory::Init() )
    {
        return false;
    }

    // Init Application
    GApplication = CreateApplication();
    Assert( GApplication != nullptr );

    Platform::SetCallbacks( &GEngine );

    if ( !GApplication->Init() )
    {
        return false;
    }

    if ( !InputManager::Get().Init() )
    {
        return false;
    }

    if ( !GRenderer.Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to create Renderer" );
        return false;
    }

    GConsole.Init();

    if ( !DebugUI::Init() )
    {
		PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to create ImGuiContext" );
        return false;
    }

    Editor::Init();

    return true;
}

void EngineLoop::Tick( Timestamp Deltatime )
{
    TRACE_FUNCTION_SCOPE();

    Platform::Tick();

    GApplication->Tick( Deltatime );

    GConsole.Tick();

    Editor::Tick();

    Profiler::Tick();

    GRenderer.Tick( *GApplication->CurrentScene );
}

void EngineLoop::Run()
{
    Timer Timer;

    while ( GEngine.IsRunning )
    {
        Timer.Tick();
        EngineLoop::Tick( Timer.GetDeltaTime() );
    }
}

bool EngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    GCmdListExecutor.WaitForGPU();

    TextureFactory::Release();

    if ( GApplication && GApplication->Release() )
    {
        SafeDelete( GApplication );
    }
    else
    {
        return false;
    }

    if ( GEngine.Release() )
    {
        Platform::SetCallbacks( nullptr );
    }
    else
    {
        return false;
    }

    DebugUI::Release();

    GRenderer.Release();

    RenderLayer::Release();

    TaskManager::Get().Release();

    if ( !Platform::Release() )
    {
        return false;
    }

    SafeDelete( GConsoleOutput );

    return true;
}
