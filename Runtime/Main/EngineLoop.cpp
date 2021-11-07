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
#include "Core/Threading/DispatchQueue.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/InterlockedInt.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"
#include "Core/Misc/EngineLoopDelegates.h"
#include "Core/Misc/EngineLoopTicker.h"

#include "Interface/InterfaceApplication.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"

#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"

bool CEngineLoop::PreInit()
{
    CFrameProfiler::Enable();

    TRACE_FUNCTION_SCOPE();

    /* Init output console */
    NErrorDevice::ConsoleWindow = PlatformConsoleWindow::Make();
    if ( !NErrorDevice::ConsoleWindow )
    {
        return false;
    }
    else
    {
        NErrorDevice::ConsoleWindow->SetTitle( PREPROCESS_CONCAT( PROJECT_NAME, ": Error Console" ) );
    }

    /* Console */
    CConsoleManager::Init();

    /* Create the actual application */
    if ( !CInterfaceApplication::Make() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create Application" );
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

    // Notify systems that the RHI is loaded
    NEngineLoopDelegates::PostInitRHIDelegate.Broadcast();

    // Init GPU Profiler
    if ( !CGPUProfiler::Init() )
    {
        LOG_ERROR( "CGPUProfiler failed to be initialized" );
    }

    if ( !CTextureFactory::Init() )
    {
        return false;
    }

    // Notify systems that the PreInit phase is over
    NEngineLoopDelegates::PreInitFinishedDelegate.Broadcast();

    return true;
}

bool CEngineLoop::Init()
{
    // Notify systems that the Engine is about to be created
    NEngineLoopDelegates::PreEngineInitDelegate.Broadcast();

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

    // Notify systems that the Engine is was initialized
    NEngineLoopDelegates::PreEngineInitDelegate.Broadcast();

    // Init Renderer
    if ( !GRenderer.Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to create Renderer" );
        return false;
    }

    // Notify systems that the Application is going to be loaded
    NEngineLoopDelegates::PreApplicationLoadedDelegate.Broadcast();

    // Init Application Module // TODO: Do not have the name hardcoded
    GApplicationModule = CModuleManager::Get().LoadEngineModule<CApplicationModule>( "Sandbox.dll" );
    if ( !GApplicationModule )
    {
        LOG_WARNING( "Application Init failed, may not behave as intended" );
    }
    else
    {
        // Notify systems that the Application is was loaded successfully
        NEngineLoopDelegates::PostApplicationLoadedDelegate.Broadcast();
    }

    // Init the interface renderer
    IInterfaceRendererModule* InterfaceRendererModule = CModuleManager::Get().LoadEngineModule<IInterfaceRendererModule>( "InterfaceRenderer" );
    if ( !InterfaceRendererModule )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to load InterfaceRenderer" );
        return false;
    }

    TSharedRef<IInterfaceRenderer> InterfaceRenderer = InterfaceRendererModule->CreateRenderer();
    if ( !InterfaceRenderer )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "FAILED to create InterfaceRenderer" );
        return false;
    }

    CInterfaceApplication::Get().SetRenderer( InterfaceRenderer );

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
    CInterfaceApplication::Get().Tick( Deltatime );

    // Tick all the registered systems
    CEngineLoopTicker::Get().Tick( Deltatime );

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

    CInterfaceApplication::Get().SetRenderer( nullptr );

    GEngine->Release();
    SafeDelete( GEngine );

    CTextureFactory::Release();

    ReleaseRHI();

    CModuleManager::Get().ReleaseAllModules();

    CDispatchQueue::Get().Release();

    CInterfaceApplication::Release();

    SafeRelease( NErrorDevice::ConsoleWindow );

    return true;
}
