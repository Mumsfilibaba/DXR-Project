#include "Engine.h"
#include "EngineLoop.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/Resources/TextureFactory.h"

#include "Editor/Editor.h"

#include "Core/Memory/Memory.h"
#include "Core/Engine/EngineGlobals.h"
#include "Core/Application/ApplicationModule.h"
#include "Core/Application/Platform/PlatformApplication.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Application/Platform/PlatformOutputConsole.h"
#include "Core/Application/Application.h"
#include "Core/Debug/Profiler.h"
#include "Core/Debug/Console/Console.h"
#include "Core/Threading/TaskManager.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/InterlockedInt.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

CEngineLoop GEngineLoop;

CEngineLoop::CEngineLoop()
{
}

CEngineLoop::~CEngineLoop()
{
}

bool CEngineLoop::PreInit()
{
    TRACE_FUNCTION_SCOPE();

    /* Init console */
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

    /* Create the platform application */
    TSharedPtr<CGenericApplication> PlatformApplication = PlatformApplication::Make();
    if ( PlatformApplication && !PlatformApplication->Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create PlatformApplication" );
        return false;
    }

    /* Create the actual application */
    TSharedPtr<CApplication> Application = CApplication::Make( PlatformApplication );
    if ( !Application )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create MainApplication" );
        return false;
    }

    if ( !TaskManager::Get().Init() )
    {
        return false;
    }

    // Create the engine 
    CEngine::Make();
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

    // Init Application Plug-In
    GApplicationModule = CreateApplicationModule();
    if ( GApplicationModule && !GApplicationModule->Init() )
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

bool CEngineLoop::Init()
{
    return true;
}

void CEngineLoop::Tick( CTimestamp Deltatime )
{
    TRACE_FUNCTION_SCOPE();

    CApplication::Get().Tick( Deltatime );

    GApplicationModule->Tick( Deltatime );

    GConsole.Tick();

    LOG_INFO( "Tick: " + std::to_string( Deltatime.AsMilliSeconds() ) + "ms" );

    Editor::Tick();

    Profiler::Tick();

    GRenderer.Tick( *GApplicationModule->CurrentScene );
}

bool CEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    GCmdListExecutor.WaitForGPU();

    TextureFactory::Release();

    if ( GApplicationModule && GApplicationModule->Release() )
    {
        SafeDelete( GApplicationModule );
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
