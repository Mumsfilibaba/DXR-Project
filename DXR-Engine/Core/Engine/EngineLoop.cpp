#include "Engine.h"
#include "EngineLoop.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/Resources/TextureFactory.h"

#include "Editor/Editor.h"

#include "Core/Memory/Memory.h"
#include "Core/Engine/EngineGlobals.h"
#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformApplication.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Application/Platform/PlatformOutputConsole.h"
#include "Core/Application/MainApplication.h"
#include "Core/Debug/Profiler.h"
#include "Core/Debug/Console/Console.h"
#include "Core/Threading/TaskManager.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/InterlockedInt.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

#include <chrono>

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

    TSharedPtr<CGenericApplication> PlatformApplication = PlatformApplication::Make();
    if ( PlatformApplication && !PlatformApplication->Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create PlatformApplication" );
        return false;
    }

    TSharedPtr<CMainApplication> Application = CMainApplication::Make( PlatformApplication );
    if ( !Application )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create MainApplication" );
        return false;
    }

    if ( !TaskManager::Get().Init() )
    {
        return false;
    }

    if ( !CEngine::Get().Init() )
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

void EngineLoop::Tick( CTimestamp Deltatime )
{
    TRACE_FUNCTION_SCOPE();

    CMainApplication::Get().Tick( Deltatime );

    GApplication->Tick( Deltatime );

    GConsole.Tick();

    LOG_INFO( "Tick: " + std::to_string( Deltatime.AsMilliSeconds() ) + "ms" );

    Editor::Tick();

    Profiler::Tick();

    GRenderer.Tick( *GApplication->CurrentScene );
}

void EngineLoop::Run()
{
    CTimer CTimer;

    while ( CEngine::Get().IsRunning )
    {
        CTimer.Tick();
        EngineLoop::Tick( CTimer.GetDeltaTime() );
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

    SafeRelease( GConsoleOutput );

    CEngine::Get().Release();

    return true;
}
