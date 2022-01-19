#include "EngineLoop.h"

#include "Engine/Engine.h"
#include "Engine/Project/ProjectManager.h"
#include "Engine/Resources/TextureFactory.h"

#if PROJECT_EDITOR
#include "EditorEngine.h"
#endif

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Modules/ApplicationModule.h"
#include "Core/Threading/DispatchQueue.h"
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

bool CEngineLoop::LoadCoreModules()
{
    CModuleManager& ModuleManager = CModuleManager::Get();

    IEngineModule* CoreModule = ModuleManager.LoadEngineModule("Core");
    IEngineModule* CoreApplicationModule = ModuleManager.LoadEngineModule("CoreApplication");
    if (!CoreModule || !CoreApplicationModule)
    {
        return false;
    }

    IEngineModule* InterfaceModule = ModuleManager.LoadEngineModule("Interface");
    if (!InterfaceModule)
    {
        return false;
    }

    IEngineModule* EngineModule = ModuleManager.LoadEngineModule("Engine");
    if (!EngineModule)
    {
        return false;
    }

    IEngineModule* RHIModule = ModuleManager.LoadEngineModule("RHI");
    if (!RHIModule)
    {
        return false;
    }

    IEngineModule* RendererModule = ModuleManager.LoadEngineModule("Renderer");
    if (!RendererModule)
    {
        return false;
    }

    return true;
}

bool CEngineLoop::PreInitialize()
{
    /* Init output console */
    NErrorDevice::GConsoleWindow = PlatformConsoleWindow::Make();
    if (!NErrorDevice::GConsoleWindow)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize ConsoleWindow");
        return false;
    }
    else
    {
        NErrorDevice::GConsoleWindow->Show(true);
        NErrorDevice::GConsoleWindow->SetTitle(CString(PROJECT_NAME) + ": Error Console");
    }

    // Load all core modules, these tend to not be reloadable
    if (!LoadCoreModules())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to Load Core-Modules");
        return false;
    }

    // Enable the profiler
    CFrameProfiler::Enable();

    TRACE_FUNCTION_SCOPE();

    // Init project information
    if (!CProjectManager::Initialize(PROJECT_NAME, PREPROCESS_CONCAT(ENGINE_LOCATION"/", PROJECT_NAME)))
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
        return false;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("ProjectName=" + CString(CProjectManager::GetProjectName()));
    LOG_INFO("ProjectPath=" + CString(CProjectManager::GetProjectPath()));
#endif

    // Console
    CConsoleManager::Initialize();

    // Init platform specific thread utilities
    if (!PlatformThreadMisc::Initialize())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to init PlatformThreadMisc");
        return false;
    }

    // Create the application interface
    if (!CInterfaceApplication::Make())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    // Init dispatch queue
    if (!CDispatchQueue::Get().Init())
    {
        return false;
    }

    // RenderAPI // TODO: Decide this via command line
    ERHIModule RenderApi =
#if PLATFORM_MACOS
        ERHIModule::Null;
#else
        ERHIModule::D3D12;
#endif
    if (!InitRHI(RenderApi))
    {
        return false;
    }

    // Notify systems that the RHI is loaded
    NEngineLoopDelegates::PostInitRHIDelegate.Broadcast();

    // Init GPU Profiler
    if (!CGPUProfiler::Init())
    {
        LOG_ERROR("CGPUProfiler failed to be initialized");
    }

    if (!CTextureFactory::Init())
    {
        return false;
    }

    // Notify systems that the PreInit phase is over
    NEngineLoopDelegates::PreInitFinishedDelegate.Broadcast();

    return true;
}

bool CEngineLoop::Initialize()
{
    // Notify systems that the Engine is about to be created
    NEngineLoopDelegates::PreEngineInitDelegate.Broadcast();

    // Create the engine
#if PROJECT_EDITOR
    GEngine = CEditorEngine::Make();
#else
    GEngine = CEngine::Make();
#endif
    if (!GEngine->Init())
    {
        return false;
    }

    // Notify systems that the Engine is was initialized
    NEngineLoopDelegates::PreEngineInitDelegate.Broadcast();

    // Init Renderer
    if (!GRenderer.Init())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    // Notify systems that the Application is going to be loaded
    NEngineLoopDelegates::PreApplicationLoadedDelegate.Broadcast();

    // Init Application Module
    GApplicationModule = CModuleManager::Get().LoadEngineModule<CApplicationModule>(CProjectManager::GetProjectModuleName());
    if (!GApplicationModule)
    {
        LOG_WARNING("Application Init failed, may not behave as intended");
    }
    else
    {
        // Notify systems that the Application is was loaded successfully
        NEngineLoopDelegates::PostApplicationLoadedDelegate.Broadcast();
    }

    // Init the interface renderer
    IInterfaceRendererModule* InterfaceRendererModule = CModuleManager::Get().LoadEngineModule<IInterfaceRendererModule>("InterfaceRenderer");
    if (!InterfaceRendererModule)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to load InterfaceRenderer");
        return false;
    }

    TSharedRef<IInterfaceRenderer> InterfaceRenderer = InterfaceRendererModule->CreateRenderer();
    if (!InterfaceRenderer)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to create InterfaceRenderer");
        return false;
    }

    CInterfaceApplication::Get().SetRenderer(InterfaceRenderer);

    // Start the engine
    if (!GEngine->Start())
    {
        return false;
    }

    return true;
}

void CEngineLoop::Tick(CTimestamp Deltatime)
{
    TRACE_FUNCTION_SCOPE();

    // Application and event-handling
    CInterfaceApplication::Get().Tick(Deltatime);

    // Tick all the registered systems
    CEngineLoopTicker::Get().Tick(Deltatime);

    // Run the engine, which means that all scene data etc. is updated
    GEngine->Tick(Deltatime);

    // Update the profiler
    CFrameProfiler::Get().Tick();

    // Update the GPUProfiler
    CGPUProfiler::Get().Tick();

    // Finally render
    GRenderer.Tick(*GEngine->Scene);
}

bool CEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    CRHICommandQueue::Get().WaitForGPU();

    CGPUProfiler::Release();

    GRenderer.Release();

    if (CInterfaceApplication::IsInitialized())
    {
        CInterfaceApplication::Get().SetRenderer(nullptr);
    }

    // Release the engine. Protect against failed initialization where the global pointer was never initialized
    if (GEngine)
    {
        GEngine->Release();

        delete GEngine;
        GEngine = nullptr;
    }

    CTextureFactory::Release();

    ReleaseRHI();

    CDispatchQueue::Get().Release();

    CInterfaceApplication::Release();

    PlatformThreadMisc::Release();

    // Release all modules at this point
    CModuleManager::Release();

    SafeRelease(NErrorDevice::GConsoleWindow);

    return true;
}
