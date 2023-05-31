#include "EngineLoop.h"
#if PROJECT_EDITOR
    #include "EditorEngine.h"
#endif
#include "Core/Modules/ModuleManager.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Threading/AsyncThreadPool.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/EngineLoopTicker.h"
#include "Core/Misc/OutputDeviceConsole.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/EngineConfig.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Project/ProjectManager.h"
#include "Application/Application.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"
#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"
#include "RHI/RHIShaderCompiler.h"
#include "Engine/Engine.h"
#include "RendererCore/TextureFactory.h"

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Launch);

FEngineLoop::FEngineLoop()
    : FrameTimer()
    , ConsoleWindow(nullptr)
{
}

FEngineLoop::~FEngineLoop()
{ 
    ConsoleWindow = nullptr;
}

bool FEngineLoop::LoadCoreModules()
{
    FModuleManager& ModuleManager = FModuleManager::Get();

    FModuleInterface* CoreModule = ModuleManager.LoadModule("Core");
    if (!CoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* CoreApplicationModule = ModuleManager.LoadModule("CoreApplication");
    if (!CoreApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* ApplicationModule = ModuleManager.LoadModule("Application");
    if (!ApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* EngineModule = ModuleManager.LoadModule("Engine");
    if (!EngineModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* RHIModule = ModuleManager.LoadModule("RHI");
    if (!RHIModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* RendererCoreModule = ModuleManager.LoadModule("RendererCore");
    if (!RendererCoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* RendererModule = ModuleManager.LoadModule("Renderer");
    if (!RendererModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* ProjectModule = ModuleManager.LoadModule("Project");
    if (!ProjectModule)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}


bool FEngineLoop::PreInitialize()
{
    // Create the console window
    ConsoleWindow = FPlatformApplicationMisc::CreateOutputDeviceConsole();
    if (ConsoleWindow)
    {
        FOutputDeviceLogger::Get()->AddOutputDevice(ConsoleWindow);  
        ConsoleWindow->Show(true);
        ConsoleWindow->SetTitle("DXR-Engine Output Console");
    }
    else
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize ConsoleWindow");
        return false;
    }

    // Load the Core-Modules
    if (!LoadCoreModules())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to load Core-Modules");
        return false;
    }

    // TODO: Use a separate profiler for booting the engine
    FFrameProfiler::Enable();
    TRACE_FUNCTION_SCOPE();

    // Initialize the engine config
    if (!FConfig::Initialize())
    {
        LOG_ERROR("Failed to initialize EngineConfig");
        return false;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
#endif

    // ProjectManager
    if (!FProjectManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
        return false;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("ProjectName=%s", FProjectManager::Get().GetProjectName().GetCString());
    LOG_INFO("ProjectPath=%s", FProjectManager::Get().GetProjectPath().GetCString());
#endif

    if (!FThreadManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ThreadManager");
        return false;
    }

    if (!FWindowedApplication::Create())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    NCoreDelegates::PostApplicationCreateDelegate.Broadcast();

    // Initialize the Async-worker threads
    if (!FAsyncThreadPool::Initialize())
    {
        return false;
    }

    // Initialize the ShaderCompiler before RHI since RHI might need to compile shaders
    if (!FRHIShaderCompiler::Create(FProjectManager::Get().GetAssetPath()))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to Initializer ShaderCompiler");
        return false;
    }

    // Initialize the RHI
    if (!RHIInitialize())
    {
        return false;
    }

    // Startup RHI Thread
    if (!FRHIThread::Startup())
    {
        LOG_ERROR("Failed to startup RHI-Thread");
        return false;
    }

    NCoreDelegates::PostInitRHIDelegate.Broadcast();

    if (!FGPUProfiler::Init())
    {
        LOG_ERROR("FGPUProfiler failed to be initialized");
    }

    if (!FTextureFactory::Init())
    {
        return false;
    }

    NCoreDelegates::PreInitFinishedDelegate.Broadcast();
    return true;
}


bool FEngineLoop::Initialize()
{
    NCoreDelegates::PreEngineInitDelegate.Broadcast();

#if PROJECT_EDITOR
    GEngine = FEditorEngine::Make();
#else
    GEngine = new FEngine();
#endif
    if (!GEngine->Init())
    {
        LOG_ERROR("Failed to initialize engine");
        return false;
    }

    NCoreDelegates::PreEngineInitDelegate.Broadcast();

    // Initialize renderer
    if (!FRenderer::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    NCoreDelegates::PreApplicationLoadedDelegate.Broadcast();

    // Load application
    GGameModule = FModuleManager::Get().LoadModule<FGameModule>(FProjectManager::Get().GetProjectModuleName().GetCString());
    if (!GGameModule)
    {
        LOG_WARNING("Failed to load Game-module, the application may not behave as intended");
    }
    else
    {
        NCoreDelegates::PostGameModuleLoadedDelegate.Broadcast();
    }

    // Prepare Application for Rendering
    if (FWindowedApplication::IsInitialized())
    {
        if (!FWindowedApplication::Get().InitializeRenderer())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to initialize RHI layer for the Application");
            return false;
        }
    }

    // Final thing is to startup the engine
    if (!GEngine->Start())
    {
        return false;
    }

    return true;
}


void FEngineLoop::Tick()
{
    TRACE_FUNCTION_SCOPE();

    // Tick the timer
    FrameTimer.Tick();

    // Poll inputs and handle events from the system
    FWindowedApplication::Get().Tick(FrameTimer.GetDeltaTime());

    // Tick all systems that have hooked into the EngineLoop::Tick
    FEngineLoopTicker::Get().Tick(FrameTimer.GetDeltaTime());

    // Tick the engine (Actors etc.)
    GEngine->Tick(FrameTimer.GetDeltaTime());

    // Tick Profiler
    FFrameProfiler::Get().Tick();

    // Tick GPU-Profiler
    FGPUProfiler::Get().Tick();

    // Tick the renderer
    FRenderer::Get().Tick(*GEngine->Scene);
}


bool FEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    // Wait for the last RHI commands to finish
    GRHICommandExecutor.WaitForGPU();

    // Release GPU profiler
    FGPUProfiler::Release();

    // Release the renderer
    FRenderer::Release();

    // Release the Application. Protect against failed initialization where the global pointer was never initialized
    if (FWindowedApplication::IsInitialized())
    {
        FWindowedApplication::Get().ReleaseRenderer();
    }

    // Release the Engine. Protect against failed initialization where the global pointer was never initialized
    if (GEngine)
    {
        GEngine->Release();
        delete GEngine;
        GEngine = nullptr;
    }

    // Release all RHI resources
    FTextureFactory::Release();

    // Wait for RHI thread and shutdown RHI Layer
    FRHIThread::Shutdown();
    RHIRelease();

    // Destroy the ShaderCompiler
    FRHIShaderCompiler::Destroy();

    // Shutdown the Async-task system
    FAsyncThreadPool::Release();

    FWindowedApplication::Destroy();

    FThreadManager::Release();

    FConfig::Release();

    SAFE_DELETE(ConsoleWindow);

    // Release all modules
    FModuleManager::Shutdown();

    return true;
}
