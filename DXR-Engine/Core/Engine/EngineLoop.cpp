#include "Engine.h"
#include "EngineLoop.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/Resources/TextureFactory.h"

#include "Editor/Editor.h"

#include "Core/Memory/Memory.h"
#include "Core/Engine/EngineGlobals.h"
#include "Core/Application/Application.h"
#include "Core/Application/Platform/Platform.h"
#include "Core/Application/Platform/PlatformApplication.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Application/Platform/PlatformOutputConsole.h"
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

    GConsoleOutput = PlatformOutputConsole::Make();
    if ( !GConsoleOutput )
    {
        return false;
    }
    else
    {
        GConsoleOutput->SetTitle( "DXR-Engine Error Console" );
    }

    Profiler::Init();

    CGenericApplication* Application = PlatformApplication::Make();
    if ( Application && !Application->Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create PlatformApplication" );
        return false;
    }

    if ( !TaskManager::Get().Init() )
    {
        return false;
    }

    GEngine = MakeShared<Engine>();
    Application->SetMessageListener( GEngine );

    if ( !GEngine->Init( Application ) )
    {
        return false;
    }

    // RenderAPI
    ERenderLayerApi RenderApi =
    #if defined(PLATFORM_MACOS)
        ERenderLayerApi::Unknown;
#else
        ERenderLayerApi::D3D12;
#endif
    if ( !RenderLayer::Init( RenderApi ) )
    {
        return false;
    }

    if ( !TextureFactory::Init() )
    {
        return false;
    }

    // Init Application
    GApplication = CreateApplication();
    if ( GApplication && !GApplication->Init() )
    {
        LOG_WARNING( "Application Init failed, may not behave as intended" );
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

    GEngine->Application->Tick( Deltatime.AsMilliSeconds() );

    GApplication->Tick( Deltatime );

    GConsole.Tick();

    LOG_INFO( "Tick" );

    Editor::Tick();

    Profiler::Tick();

    GRenderer.Tick( *GApplication->CurrentScene );
}

void EngineLoop::Run()
{
    Timer Timer;

    while ( GEngine->IsRunning )
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

    DebugUI::Release();

    GRenderer.Release();

    RenderLayer::Release();

    TaskManager::Get().Release();

    SafeDelete( GConsoleOutput );

    GEngine->Release();
    GEngine.Reset();

    return true;
}
