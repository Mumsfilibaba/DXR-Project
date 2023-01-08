#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/Project/ProjectManager.h"
#include "Engine/Resources/TextureFactory.h"
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
#include "Application/Application.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"
#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"
#include "RHI/RHIShaderCompiler.h"

FEngineLoop::FEngineLoop()
    : FrameTimer()
    , ConsoleWindow(nullptr)
{ }

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

    FModuleInterface* RendererModule = ModuleManager.LoadModule("Renderer");
    if (!RendererModule)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}


bool FEngineLoop::PreInit()
{
    // Create the console window
    ConsoleWindow = FPlatformApplicationMisc::CreateOutputDeviceConsole();
    if (ConsoleWindow)
    {
        FOutputDeviceLogger::Get()->AddOutputDevice(ConsoleWindow);
        
        ConsoleWindow->Show(true);
        ConsoleWindow->SetTitle(FString(PROJECT_NAME) + ": Error Console");
    }
    else
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize ConsoleWindow");
        return false;
    }

    // Load the Core-Modules
    if (!LoadCoreModules())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to Load Core-Modules");
        return false;
    }

    // Initialize the config system
    if (!FConfig::Initialize())
    {
        LOG_ERROR("Failed to initialize EngineConfig");
        return false;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
#endif

    // TODO: Use a separate profiler for booting the engine
    FFrameProfiler::Enable();
    TRACE_FUNCTION_SCOPE();

    // ProjectManager
    {
        const FString ProjectLocation     = FString(ENGINE_LOCATION) + FString("/") + FString(PROJECT_NAME);
        const FString AssetFolderLocation = FString(ENGINE_LOCATION) + FString("/Assets");
        if (!FProjectManager::Initialize(PROJECT_NAME, ProjectLocation.GetCString(), AssetFolderLocation.GetCString()))
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
            return false;
        }
    }

#if !PRODUCTION_BUILD
    LOG_INFO("ProjectName=%s", FProjectManager::GetProjectName());
    LOG_INFO("ProjectPath=%s", FProjectManager::GetProjectPath());
#endif

    if (!FThreadManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ThreadManager");
        return false;
    }

    if (!FApplication::Create())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    // Initialize the Async-worker threads
    {
        const auto NumProcessors = FPlatformThreadMisc::GetNumProcessors();
        if (!FAsyncThreadPool::Initialize(NumProcessors))
        {
            return false;
        }
    }

    // Initialize the ShaderCompiler before RHI since RHI might need to compile shaders
    if (!FRHIShaderCompiler::Create(FProjectManager::GetAssetPath()))
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
    {
        if (!FRHIThread::Startup())
        {
            LOG_ERROR("Failed to startup RHI-Thread");
            return false;
        }
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


bool FEngineLoop::Init()
{
    NCoreDelegates::PreEngineInitDelegate.Broadcast();

#if PROJECT_EDITOR
    GEngine = FEditorEngine::Make();
#else
    GEngine = new FEngine();
#endif
    if (!GEngine->Initialize())
    {
        LOG_ERROR("Failed to initialize engine");
        return false;
    }

    NCoreDelegates::PreEngineInitDelegate.Broadcast();

    // Initialize renderer
    {
        if (!FRenderer::Initialize())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
            return false;
        }
    }

    NCoreDelegates::PreApplicationLoadedDelegate.Broadcast();

    // Load application
    {
        GApplicationModule = FModuleManager::Get().LoadModule<FApplicationModule>(FProjectManager::GetProjectModuleName());
        if (!GApplicationModule)
        {
            LOG_WARNING("Application Init failed, may not behave as intended");
        }
        else
        {
            NCoreDelegates::PostApplicationLoadedDelegate.Broadcast();
        }
    }

    if (FApplication::IsInitialized())
    {
        if (!FApplication::Get().InitializeRHI())
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

    FApplication::Get().Tick(FrameTimer.GetDeltaTime());

    FEngineLoopTicker::Get().Tick(FrameTimer.GetDeltaTime());

    GEngine->Tick(FrameTimer.GetDeltaTime());

    FFrameProfiler::Get().Tick();

    FGPUProfiler::Get().Tick();

    FRenderer::Get().Tick(*GEngine->Scene);
}


bool FEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    GRHICommandExecutor.WaitForGPU();

    FGPUProfiler::Release();

    FRenderer::Release();

    // Release the Application. Protect against failed initialization where the global pointer was never initialized
    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseRHI();
    }

    // Release the Engine. Protect against failed initialization where the global pointer was never initialized
    if (GEngine)
    {
        GEngine->Release();

        GEngine->Destroy();
        GEngine = nullptr;
    }

    FTextureFactory::Release();

    {
        FRHIThread::Shutdown();
    }

    RHIRelease();

    FRHIShaderCompiler::Destroy();

    FAsyncThreadPool::Release();

    FApplication::Destroy();

    FThreadManager::Release();

    FConfig::Release();

    SAFE_DELETE(ConsoleWindow);

    FModuleManager::Shutdown();
    return true;
}
