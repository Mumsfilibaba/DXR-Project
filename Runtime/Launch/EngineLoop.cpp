#include "Launch/EngineLoop.h"
#include "Core/CoreGlobals.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Tasks/TaskGraph.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/EngineConfig.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/FileOutputDevice.h"
#include "Application/Application.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleOutputDevice.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Engine.h"
#include "RendererCore/TextureFactory.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

IMPLEMENT_ENGINE_MODULE(IModule, Launch);

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FDebuggerOutputDevice : public IOutputDevice
{
    virtual void Log(const String& Message)
    {
        FPlatformMisc::OutputDebugString(*Message);
    }

    virtual void Log(ELogSeverity Severity, const String& Message)
    {
        FPlatformMisc::OutputDebugString(*Message);
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

static TUniquePtr<FDebuggerOutputDevice>       GDebuggerOutputDevice;
static TUniquePtr<FGenericConsoleOutputDevice> GConsoleWindow;
static TUniquePtr<FFileOutputDevice>           GFileOutputDevice;

static bool InitializeOutputDevices()
{
    // Create the console window
    GConsoleWindow = TUniquePtr<FGenericConsoleOutputDevice>(FPlatformConsoleOutputDevice::Create());
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

    const String OutputLogPath = Paths::GetProjectDir() + "/OutputLog.txt";
    GFileOutputDevice = MakeUniquePtr<FFileOutputDevice>(OutputLogPath);
    
    if (GFileOutputDevice && GFileOutputDevice->IsValid())
    {
        FOutputDeviceLogger::Get()->RegisterOutputDevice(GFileOutputDevice.Get());
    }

    return true;
}

static void LogStartupInformation()
{
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
    LOG_INFO("ProjectName=%s", *Paths::GetProjectName());
    LOG_INFO("ProjectDir=%s", *Paths::GetProjectDir());
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
    IModule* CoreModule = FModuleManager::Get().LoadModule("Core");
    if (!CoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* CoreApplicationModule = FModuleManager::Get().LoadModule("CoreApplication");
    if (!CoreApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* ApplicationModule = FModuleManager::Get().LoadModule("Application");
    if (!ApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* EngineModule = FModuleManager::Get().LoadModule("Engine");
    if (!EngineModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* RHIModule = FModuleManager::Get().LoadModule("RHI");
    if (!RHIModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* RendererCoreModule = FModuleManager::Get().LoadModule("RendererCore");
    if (!RendererCoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* RendererModule = FModuleManager::Get().LoadModule("Renderer");
    if (!RendererModule)
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

    if (!CommandLine::Initialize(Args, NumArgs))
    {
        LOG_WARNING("Invalid CommandLine");
    }

    GIsUnattended = CommandLine::FindOption("unattended");

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

    FConsoleManager::Get().LoadConsoleVariablesFromCommandLine();

    if (!FThreadManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ThreadManager");
        return -1;
    }

    if (!FApplication::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return -1;
    }

    CoreDelegates::PostApplicationCreateDelegate.Broadcast();

    // Initialize the task graph (named-thread lanes + anonymous worker pool)
    if (!FTaskGraph::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize TaskGraph");
        return -1;
    }

    if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to Initializer ShaderCompiler");
        return -1;
    }

    if (!RHI::Initialize())
    {
        return -1;
    }

    CoreDelegates::PostInitRHIDelegate.Broadcast();

    if (!FTextureFactory::Initialize())
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

    if (!FEngine::Initialize())
    {
        LOG_ERROR("Failed to initialize engine");
        return -1;
    }

    // Log some startup information after the engine is loaded
    LogStartupInformation();

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

    // Init Engine resource that needs the renderer to be initialized
	if (!FEngine::Get()->InitPostRenderer())
	{
		return -1;
	}

    // Start the engine
    if (!FEngine::Get()->Start())
    {
        return -1;
    }

    return 0;
}

void FEngineLoop::Tick()
{
    TRACE_FUNCTION_SCOPE();

    // Run any work that was queued onto the main thread since the last tick.
    Tasks::ProcessMainThreadTasks();

    // Tick the timer
    FrameTimer.Tick();

    const float DeltaTime = static_cast<float>(FrameTimer.GetDeltaTime().AsSeconds());
    FApplication::Get().Tick(DeltaTime);

    // The window-close message is pumped above; once exit is requested the surface may already be gone.
    if (IsEngineExitRequested())
    {
        return;
    }

    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->FinishPreviousFrame();

    FEngine::Get()->Tick(DeltaTime);

    RendererModule->RecordUI();
    RendererModule->Tick();

    FSceneRenderPacket Packet = FEngine::Get()->BuildRenderPacket();
    RendererModule->KickSceneRender(::Move(Packet));

    FFrameProfiler::Get().Tick();
}

void FEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    // Drain the one-frame-ahead pipeline without presenting.
    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->DiscardPendingFrame();
    }

    // Wait for the last RHI commands to finish
    if (FRHICommandListExecutor::IsInitialized())
    {
        FRHICommandListExecutor::Get().WaitForGPU();
    }

    // Release the renderer
    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->Release();
    }

    // Destroy the Engine
    FEngine::Destroy();

    // Unload ModuleManager
    if (IImguiPlugin::IsEnabled())
    {
        FModuleManager::Get().UnloadModule("ImGuiPlugin");
    }

    // Release all RHI resources
    FTextureFactory::Release();

    // Wait for RHI thread and shutdown RHI Layer
    RHI::Release();

    FShaderCompiler::Destroy();

    // Shut down the task graph workers.
    FTaskGraph::Release();

    FApplication::Release();

    FThreadManager::Release();

    FConfig::Release();

    // Clear all core delegates before unloading modules to prevent dangling vtable pointers
    CoreDelegates::Shutdown();

    // Release all modules
    FModuleManager::Shutdown();

    if (FPlatformMisc::IsDebuggerPresent())
    {
        GMalloc->DumpAllocations(GDebuggerOutputDevice.Get());
    }
}
