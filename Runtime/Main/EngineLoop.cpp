#include "EngineLoop.h"

#include "Engine/Engine.h"
#include "Engine/Resources/TextureFactory.h"

#if PROJECT_EDITOR
#include "EditorEngine.h"
#endif

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Memory/Memory.h"
#include "Core/Modules/ModuleManger.h"
#include "Core/Modules/ApplicationModule.h"
#include "Core/Application/Platform/PlatformApplication.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Application/Platform/PlatformOutputConsole.h"
#include "Core/Application/Application.h"
#include "Core/Threading/DispatchQueue.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/InterlockedInt.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

#include "Renderer/UIRenderer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"

bool CEngineLoop::PreInit()
{
    CFrameProfiler::Enable();

    TRACE_FUNCTION_SCOPE();

    /* Init output console */
    GConsoleOutput = PlatformOutputConsole::Make();
    if ( !GConsoleOutput )
    {
        return false;
    }
    else
    {
        GConsoleOutput->SetTitle( PREPROCESS_CONCAT( PROJECT_NAME, ": Error Console" ) );
    }

    /* Console */
    CConsoleManager::Init();

    /* Create the actual application */
    if ( !CApplication::Make() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create MainApplication" );
        return false;
    }

    /* Init dispatch queue */
    if ( !CDispatchQueue::Get().Init() )
    {
        return false;
    }

    // RenderAPI // TODO: Decide this via command line
    ERHIModule RenderApi =
    #if defined(PLATFORM_MACOS)
        ERHIModule::Null;
#else
        ERHIModule::D3D12;
#endif
    if ( !InitRHI( RenderApi ) )
    {
        return false;
    }

    if ( !CGPUProfiler::Init() )
    {
        LOG_ERROR( "CGPUProfiler failed to be initialized" );
    }

    if ( !CTextureFactory::Init() )
    {
        return false;
    }

    return true;
}

bool CEngineLoop::Init()
{
    // Create the engine
#if PROJECT_EDITOR
    GEngine = CEditorEngine::Make();
#else
    GEngine = CEngine::Make();
#endif
    if ( !GEngine->Init() )
    {
        return false;
    }

    if ( !GRenderer.Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to create Renderer" );
        return false;
    }

    // Init Application Module // TODO: Do not have the name hardcoded
    GApplicationModule = CModuleManager::Get().LoadEngineModule<CApplicationModule>( "Sandbox.dll" );
    if ( !GApplicationModule )
    {
        LOG_WARNING( "Application Init failed, may not behave as intended" );
    }

    // UI // TODO: Has to be initialized after the engine, however, there should be a delegate on the application that notifies when a viewport is registered*/
    TSharedRef<IUIRenderer> UIRenderer = CUIRenderer::Make();
    if ( !UIRenderer )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to create UIRenderer" );
        return false;
    }

    CApplication::Get().SetRenderer( UIRenderer );

    // Start the engine
    if ( !GEngine->Start() )
    {
        return false;
    }

    return true;
}

void CEngineLoop::Tick( CTimestamp Deltatime )
{
    TRACE_FUNCTION_SCOPE();

    // Application and event-handling
    CApplication::Get().Tick( Deltatime );

    // TODO: This should be bound via delegates?
    GApplicationModule->Tick( Deltatime );

    LOG_INFO( "Tick: " + ToString( Deltatime.AsMilliSeconds() ) );

    // Run the engine, which means that all scene data etc. is updated
    GEngine->Tick( Deltatime );

    // Update the profiler
    CFrameProfiler::Get().Tick();

    // Update the GPUProfiler
    CGPUProfiler::Get().Tick();

    // Finally render
    GRenderer.Tick( *GEngine->Scene );
}

bool CEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    CRHICommandQueue::Get().WaitForGPU();

    CGPUProfiler::Release();

    GRenderer.Release();

    CApplication::Get().SetRenderer( nullptr );

    GEngine->Release();
    SafeDelete( GEngine );

    CTextureFactory::Release();

    ReleaseRHI();

    CModuleManager::Get().ReleaseAllModules();

    CDispatchQueue::Get().Release();

    CApplication::Release();

    SafeRelease( GConsoleOutput );

    return true;
}
