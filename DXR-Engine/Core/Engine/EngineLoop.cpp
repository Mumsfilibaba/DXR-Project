#include "Engine.h"
#include "EngineLoop.h"

#include "Rendering/UIRenderer.h"
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
#include "Core/Debug/Console/ConsoleManager.h"
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

    /* Profiler */
    CProfiler::Init();

    /* Create the platform application */
    TSharedPtr<CCoreApplication> PlatformApplication = PlatformApplication::Make();
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

    /* Console */ // TODO: Separate panel from console (CConsoleManager, and CConsolePanel)
    GConsole.Init();

    if ( !CTaskManager::Get().Init() )
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
    if ( !CRHIModule::Init( RenderApi ) )
    {
        return false;
    }

    if ( !CTextureFactory::Init() )
    {
        return false;
    }

    // Create the engine 
    GEngine = CEngine::Make();
    if ( !GEngine->Init() )
    {
        return false;
    }

    /* UI */
    if ( !CUIRenderer::Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to create ImGuiContext" );
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

    Editor::Init();

    return true;
}

bool CEngineLoop::Init()
{
    if ( !GEngine->Start() )
    {
        return false;
    }

    return true;
}

void CEngineLoop::Tick( CTimestamp Deltatime )
{
    TRACE_FUNCTION_SCOPE();

    CApplication::Get().Tick( Deltatime );

    GApplicationModule->Tick( Deltatime );

    GConsole.Tick();

    GEngine->Tick( Deltatime );

    LOG_INFO( "Tick: " + ToString( Deltatime.AsMilliSeconds() ) + "ms" );

    Editor::Tick();

    CProfiler::Tick();

    GRenderer.Tick( *GEngine->Scene );
}

bool CEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    GCmdListExecutor.WaitForGPU();

    CTextureFactory::Release();

    if ( GApplicationModule && GApplicationModule->Release() )
    {
        SafeDelete( GApplicationModule );
    }
    else
    {
        return false;
    }

    CUIRenderer::Release();

    GRenderer.Release();

    GEngine->Release();
    GEngine.Reset();

    CRHIModule::Release();

    CTaskManager::Get().Release();

    SafeRelease( GConsoleOutput );

    return true;
}
