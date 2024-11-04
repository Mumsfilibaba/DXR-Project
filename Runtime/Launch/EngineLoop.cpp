#include "EngineLoop.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Threading/TaskManager.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/OutputDeviceConsole.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/EngineConfig.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CommandLine.h"
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

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FDebuggerOutputDevice : public IOutputDevice
{
    virtual void Log(const FString& Message)
    {
        FPlatformMisc::OutputDebugString(Message.GetCString());
    }

    virtual void Log(ELogSeverity Severity, const FString& Message)
    {
        FPlatformMisc::OutputDebugString(Message.GetCString());
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

static TUniquePtr<FOutputDeviceConsole>  GConsoleWindow;
static TUniquePtr<FDebuggerOutputDevice> GDebuggerOutputDevice;

static bool InitializeOutputDevices()
{
    // Create the console window
    GConsoleWindow = TUniquePtr<FOutputDeviceConsole>(FPlatformApplicationMisc::CreateOutputDeviceConsole());
    if (GConsoleWindow)
    {
        GConsoleWindow->Show(true);
        GConsoleWindow->SetTitle("DXR-Engine Output Console");
        FOutputDeviceLogger::Get()->RegisterOutputDevice(GConsoleWindow.Get());
    }
    else
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize ConsoleWindow");
        return false;
    }

    if (FPlatformMisc::IsDebuggerPresent())
    {
        GDebuggerOutputDevice = MakeUniquePtr<FDebuggerOutputDevice>();
        FOutputDeviceLogger::Get()->RegisterOutputDevice(GDebuggerOutputDevice.Get());
    }

    return true;
}


FEngineLoop::FEngineLoop()
    : FrameTimer()
{
}

FEngineLoop::~FEngineLoop()
{
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

int32 FEngineLoop::PreInit(const CHAR** Args, int32 NumArgs)
{
    if (!InitializeOutputDevices())
    {
        return -1;
    }

    if (!FCommandLine::Initialize(Args, NumArgs))
    {
        LOG_WARNING("Invalid CommandLine");
    }

    if (!LoadCoreModules())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to load Core-Modules");
        return -1;
    }

    // TODO: Use a separate profiler for booting the engine
    FFrameProfiler::Get().Enable();
    TRACE_FUNCTION_SCOPE();

    if (!FConfig::Initialize())
    {
        LOG_ERROR("Failed to initialize EngineConfig");
        return -1;
    }

    if (!FProjectManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
        return -1;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
    LOG_INFO("ProjectName=%s", FProjectManager::Get().GetProjectName().GetCString());
    LOG_INFO("ProjectPath=%s", FProjectManager::Get().GetProjectPath().GetCString());
#endif

    if (!FThreadManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ThreadManager");
        return -1;
    }

    if (!FApplicationInterface::Create())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return -1;
    }

    CoreDelegates::PostApplicationCreateDelegate.Broadcast();

    // Initialize async-worker threads
    if (!FTaskManager::Initialize())
    {
        return -1;
    }

    if (!FShaderCompiler::Create(FProjectManager::Get().GetAssetPath()))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to Initializer ShaderCompiler");
        return -1;
    }

    if (!RHIInitialize())
    {
        return -1;
    }

    CoreDelegates::PostInitRHIDelegate.Broadcast();

    if (!FTextureFactory::Init())
    {
        return -1;
    }

    CoreDelegates::PreInitFinishedDelegate.Broadcast();
    return 0;
}

int32 FEngineLoop::Init()
{
    // Initialize ImGui (Currently Required)
    IImguiPlugin* ImguiPlugin = FModuleManager::Get().LoadModule<IImguiPlugin>("ImGuiPlugin");
    if (!ImguiPlugin)
    {
        LOG_ERROR("Failed to load ImGuiPlugin");
        return -1;
    }

    CoreDelegates::PreEngineInitDelegate.Broadcast();

    GEngine = new FEngine();
    if (!GEngine->Init())
    {
        LOG_ERROR("Failed to initialize engine");
        return -1;
    }

    CoreDelegates::PreEngineInitDelegate.Broadcast();

    IRendererModule* RendererModule = IRendererModule::Get();
    if (!RendererModule->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return -1;
    }

    CoreDelegates::PreApplicationLoadedDelegate.Broadcast();

    // Prepare ImGui for Rendering
    if (IImguiPlugin::IsEnabled())
    {
        if (!IImguiPlugin::Get().InitializeRHI())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to initialize RHI resources for ImGui");
            return -1;
        }
    }

    if (!GEngine->Start())
    {
        return -1;
    }

    return 0;
}

void FEngineLoop::Tick()
{
    TRACE_FUNCTION_SCOPE();

    // Tick the timer
    FrameTimer.Tick();

    const float DeltaTime = static_cast<float>(FrameTimer.GetDeltaTime().AsSeconds());
    FApplicationInterface::Get().Tick(DeltaTime);

    GEngine->Tick(DeltaTime);

    FFrameProfiler::Get().Tick();

    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->Tick();
}

void FEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    // Wait for the last RHI commands to finish
    GRHICommandExecutor.WaitForGPU();

    // Release the renderer
    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->Release();

    // Release the Engine (Protect against failed initialization where the global pointer was never initialized)
    if (GEngine)
    {
        GEngine->Release();

        delete GEngine;
        GEngine = nullptr;
    }

    // Unload ModuleManager
    if (IImguiPlugin::IsEnabled())
    {
        FModuleManager::Get().UnloadModule("ImGuiPlugin");
    }

    // Release all RHI resources
    FTextureFactory::Release();

    // Wait for RHI thread and shutdown RHI Layer
    RHIRelease();

    FShaderCompiler::Destroy();

    FTaskManager::Release();

    FApplicationInterface::Destroy();

    FThreadManager::Release();

    FConfig::Release();

    // Release all modules
    FModuleManager::Shutdown();

    if (FPlatformMisc::IsDebuggerPresent())
    {
        GMalloc->DumpAllocations(GDebuggerOutputDevice.Get());
    }
}
