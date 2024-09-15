#include "EngineLoop.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Threading/TaskManager.h"
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
#include "Renderer/Debug/GPUProfiler.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Engine.h"
#include "RendererCore/TextureFactory.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

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
    FModuleInterface* CoreModule = FModuleManager::Get().LoadModule("Core");
    if (!CoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* CoreApplicationModule = FModuleManager::Get().LoadModule("CoreApplication");
    if (!CoreApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* ApplicationModule = FModuleManager::Get().LoadModule("Application");
    if (!ApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* EngineModule = FModuleManager::Get().LoadModule("Engine");
    if (!EngineModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* RHIModule = FModuleManager::Get().LoadModule("RHI");
    if (!RHIModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* RendererCoreModule = FModuleManager::Get().LoadModule("RendererCore");
    if (!RendererCoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* RendererModule = FModuleManager::Get().LoadModule("Renderer");
    if (!RendererModule)
    {
        DEBUG_BREAK();
        return false;
    }

    FModuleInterface* ProjectModule = FModuleManager::Get().LoadModule("Project");
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

    // Load all core-modules
    if (!LoadCoreModules())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to load Core-Modules");
        return false;
    }

    // TODO: Use a separate profiler for booting the engine
    FFrameProfiler::Get().Enable();
    TRACE_FUNCTION_SCOPE();

    // Initialize the engine config
    if (!FConfig::Initialize())
    {
        LOG_ERROR("Failed to initialize EngineConfig");
        return false;
    }

    // ProjectManager
    if (!FProjectManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
        return false;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
    LOG_INFO("ProjectName=%s", FProjectManager::Get().GetProjectName().GetCString());
    LOG_INFO("ProjectPath=%s", FProjectManager::Get().GetProjectPath().GetCString());
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

    CoreDelegates::PostApplicationCreateDelegate.Broadcast();

    // Initialize async-worker threads
    if (!FTaskManager::Initialize())
    {
        return false;
    }

    if (!FShaderCompiler::Create(FProjectManager::Get().GetAssetPath()))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to Initializer ShaderCompiler");
        return false;
    }

    if (!RHIInitialize())
    {
        return false;
    }

    CoreDelegates::PostInitRHIDelegate.Broadcast();

    if (!FTextureFactory::Init())
    {
        return false;
    }

    CoreDelegates::PreInitFinishedDelegate.Broadcast();
    return true;
}

bool FEngineLoop::Initialize()
{
    // Initialize ImGui (Currently Required)
    IImguiPlugin* ImguiPlugin = FModuleManager::Get().LoadModule<IImguiPlugin>("ImGuiPlugin");
    if (!ImguiPlugin)
    {
        LOG_ERROR("Failed to load ImGuiPlugin");
        return false;
    }

    // Initialize the engine
    CoreDelegates::PreEngineInitDelegate.Broadcast();

    GEngine = new FEngine();
    if (!GEngine->Init())
    {
        LOG_ERROR("Failed to initialize engine");
        return false;
    }

    CoreDelegates::PreEngineInitDelegate.Broadcast();

    // Initialize renderer
    IRendererModule* RendererModule = IRendererModule::Get();
    if (!RendererModule->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    CoreDelegates::PreApplicationLoadedDelegate.Broadcast();

    // Load application
    GGameModule = FModuleManager::Get().LoadModule<FGameModule>(FProjectManager::Get().GetProjectModuleName().Data());
    if (!GGameModule)
    {
        LOG_WARNING("Failed to load Game-module, the application may not behave as intended");
    }
    else
    {
        CoreDelegates::PostGameModuleLoadedDelegate.Broadcast();
    }

    // Prepare ImGui for Rendering
    if (IImguiPlugin::IsEnabled())
    {
        if (!IImguiPlugin::Get().InitializeRenderer())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to initialize RHI resources for ImGui");
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

    // DeltaTime
    const FTimespan DeltaTime = FrameTimer.GetDeltaTime();

    // Poll inputs and handle events from the system
    FApplication::Get().Tick(DeltaTime.AsMilliseconds());

    // Tick all systems that have hooked into the EngineLoop::Tick
    FEngineLoopTicker::Get().Tick(DeltaTime);

    // Tick the engine (Actors etc.)
    GEngine->Tick(DeltaTime.AsMilliseconds());

    // Tick Profiler
    FFrameProfiler::Get().Tick();

    // Tick GPU-Profiler
    FGPUProfiler::Get().Tick();

    // Tick the renderer
    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->Tick();
}

bool FEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    // Wait for the last RHI commands to finish
    GRHICommandExecutor.WaitForGPU();

    // Release GPU profiler
    FGPUProfiler::Get().Release();

    // Release ImGui
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().ReleaseRenderer();
    }

    // TODO: We need a main window, this should be de-registered here

    // Release the renderer
    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->Release();
    
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
    RHIRelease();

    FShaderCompiler::Destroy();

    FTaskManager::Release();

    FApplication::Destroy();

    FThreadManager::Release();

    FConfig::Release();

    SAFE_DELETE(ConsoleWindow);

    // Release all modules
    FModuleManager::Shutdown();
    return true;
}
