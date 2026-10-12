#include "LaunchEngine/EngineLoop.h"
#include "Core/CoreGlobals.h"
#include "Core/Memory/MemoryPagePool.h"
#include "Core/PlatformInterface/PlatformEventPool.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Tasks/TaskGraph.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Config.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/BootProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/EngineRegistry.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Misc/BuildInfo.h"
#include "Core/Misc/FileOutputDevice.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "RendererCore/Interfaces/IGPUProfiler.h"
#include "Core/Platform/PlatformMisc.h"
#include "Application/Application.h"
#include "ApplicationRenderer/ApplicationRenderer.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"
#include "RHI/RHI.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "RemoteConsole/RemoteConsoleServer.h"
#include "Engine/Engine.h"
#include "Engine/Performance/ProfileRun.h"
#include "RendererCore/RenderGraph/RenderGraphResourcePool.h"
#include "ShaderCompiler/ShaderBytecodeCache.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/VertexStreamCache.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

IMPLEMENT_ENGINE_MODULE(IModule, LaunchEngine);

DISABLE_UNREFERENCED_VARIABLE_WARNING

static bool GSynchronousResize = true;
static FAutoConsoleVariableRef CVarSynchronousResize(
    TEXT("Renderer.Feature.SynchronousResize"),
    TEXT("Repaints a window inline while it is being resized, instead of leaving the new area blank until the frame loop catches up"),
    GSynchronousResize,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarExitAfterFrames(
    TEXT("Engine.ExitAfterFrames"),
    TEXT("Requests exit once this many frames have been submitted, which is what makes a headless boot check possible. Zero runs until closed"),
    0,
    EConsoleVariableFlags::Default);
struct FDebuggerOutputDevice : public IOutputDevice
{
    virtual void Log(const FString& Message)
    {
        FPlatformMisc::OutputDebugString(*Message);
    }

    virtual void Log(ELogSeverity Severity, const FString& Message)
    {
        FPlatformMisc::OutputDebugString(*Message);
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

static TUniquePtr<FDebuggerOutputDevice>  GDebuggerOutputDevice;
static TUniquePtr<IPlatformConsoleWindow> GConsoleWindow;
static TUniquePtr<FFileOutputDevice>      GFileOutputDevice;

static bool InitializeOutputDevices()
{
    // Create the console window
    GConsoleWindow = TUniquePtr<IPlatformConsoleWindow>(FPlatformConsoleWindow::Create());
    if (GConsoleWindow)
    {
        GConsoleWindow->Show(true);
        GConsoleWindow->SetTitle(TEXT("DXR-Engine Output Console"));
        FOutputDeviceManager::Get()->RegisterOutputDevice(GConsoleWindow.Get());
    }
    else
    {
        FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("Failed to initialize ConsoleWindow"));
        return false;
    }

    if (FPlatformMisc::IsDebuggerPresent())
    {
        GDebuggerOutputDevice = MakeUniquePtr<FDebuggerOutputDevice>();
        FOutputDeviceManager::Get()->RegisterOutputDevice(GDebuggerOutputDevice.Get());
    }

    const FString OutputLogPath = Paths::GetProjectDir() + TEXT("/OutputLog.txt");
    GFileOutputDevice = MakeUniquePtr<FFileOutputDevice>(OutputLogPath);
    
    if (GFileOutputDevice && GFileOutputDevice->IsValid())
    {
        FOutputDeviceManager::Get()->RegisterOutputDevice(GFileOutputDevice.Get());
    }

    FOutputDeviceManager::Get()->FlushPendingLines();
    return true;
}

static void LogStartupInformation()
{
    LOG_INFO("Engine=%s %s", BuildInfo::GetEngineName(), BuildInfo::GetVersionString());
    LOG_INFO("Configuration=%s (%s)", BuildInfo::GetConfigurationName(), BuildInfo::GetLinkageName());
    LOG_INFO("Platform=%s %s", BuildInfo::GetPlatformName(), BuildInfo::GetArchitectureName());
    LOG_INFO("Commit=%s%s (%s)", BuildInfo::GetCommit(), BuildInfo::IsWorkingTreeDirty() ? TEXT(" (modified)") : TEXT(""), BuildInfo::GetBranch());
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? TEXT("true") : TEXT("false"));
    LOG_INFO("Executable=%s", FPlatformFile::GetExecutablePath());
    LOG_INFO("EngineDir=%s (%s)", *Paths::GetEngineDir(), Paths::GetEngineDirSourceName());
    LOG_INFO("ProjectName=%s", *Paths::GetProjectName());
    LOG_INFO("ProjectDir=%s (%s)", *Paths::GetProjectDir(), Paths::GetProjectSourceName());
    if (!Paths::GetProjectFile().IsEmpty())
    {
        LOG_INFO("ProjectFile=%s", *Paths::GetProjectFile());
    }
}

FEngineLoop::FEngineLoop()
    : UIRenderer(nullptr)
    , FrameTimer()
    , FrameCounter(0)
    , bIsRedrawingForResize(false)
{
}

FEngineLoop::~FEngineLoop()
{
}

bool FEngineLoop::LoadCoreModules()
{
    IModule* CoreModule = FModuleManager::Get().LoadModule(TEXT("Core"));
    if (!CoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* CoreApplicationModule = FModuleManager::Get().LoadModule(TEXT("CoreApplication"));
    if (!CoreApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* ApplicationModule = FModuleManager::Get().LoadModule(TEXT("Application"));
    if (!ApplicationModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* EngineModule = FModuleManager::Get().LoadModule(TEXT("Engine"));
    if (!EngineModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* RHIModule = FModuleManager::Get().LoadModule(TEXT("RHI"));
    if (!RHIModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* RendererCoreModule = FModuleManager::Get().LoadModule(TEXT("RendererCore"));
    if (!RendererCoreModule)
    {
        DEBUG_BREAK();
        return false;
    }

    IModule* RendererModule = FModuleManager::Get().LoadModule(TEXT("Renderer"));
    if (!RendererModule)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

int32 FEngineLoop::PreInit(const CHAR_T** Args, int32 NumArgs)
{
    const bool bIsCommandLineValid = CommandLine::Initialize(Args, NumArgs);
    if (!Paths::Initialize())
    {
        FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), FString::Printf(TEXT("Could not find the engine folder. Pass -engine=<path> to a folder "
            "that contains %s, or run the executable from inside one."), Paths::EngineIdentityFilename));
        return -1;
    }

    FString ProjectError;
    if (!Paths::InitializeProject(ProjectError))
    {
        FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), FString::Printf(TEXT("Could not load the project: %s"), *ProjectError));
        return -1;
    }

    if (!InitializeOutputDevices())
    {
        return -1;
    }

    // The earliest point a report has somewhere to go, and everything below here is worth having covered.
    FPlatformMisc::InstallCrashHandler();

    if (!bIsCommandLineValid)
    {
        LOG_WARNING("Invalid CommandLine");
    }

    GIsUnattended = CommandLine::FindOption(TEXT("unattended"));
    FProfileRun::ConfigureFromCommandLine();

    if (!LoadCoreModules())
    {
        FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("Failed to load Core-Modules"));
        return -1;
    }

    FBootProfiler::Get().Enable();
    FProfileRun::PrepareCapture();

    TRACE_BOOT_SCOPE(TEXT("PreInit"));

    {
        TRACE_BOOT_SCOPE(TEXT("Config"));

        if (!FConfig::Initialize())
        {
            LOG_ERROR("Failed to initialize Config");
            return -1;
        }
    }

    GConfig->LoadConsoleVariables();
    FConsoleManager::Get().LoadFromCommandLine();

    {
        TRACE_BOOT_SCOPE(TEXT("ThreadManager"));

        if (!FThreadManager::Initialize())
        {
            FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("Failed to init ThreadManager"));
            return -1;
        }
    }

    {
        TRACE_BOOT_SCOPE(TEXT("RemoteConsole"));

        if (!FRemoteConsoleServer::Initialize())
        {
            LOG_WARNING("Failed to start the RemoteConsole server, continuing without it");
        }
    }

    {
        TRACE_BOOT_SCOPE(TEXT("Application"));

        if (!FApplication::Initialize())
        {
            FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("Failed to create Application"));
            return -1;
        }
    }

    CoreDelegates::PostApplicationCreateDelegate.Broadcast();

    {
        TRACE_BOOT_SCOPE(TEXT("TaskGraph"));

        if (!FTaskGraph::Initialize())
        {
            FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("Failed to initialize TaskGraph"));
            return -1;
        }
    }

    {
        TRACE_BOOT_SCOPE(TEXT("ShaderCompiler"));

        if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
        {
            FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("Failed to Initializer ShaderCompiler"));
            return -1;
        }
    }

    {
        TRACE_BOOT_SCOPE(TEXT("RHI"));

        if (!RHI::Initialize())
        {
            LOG_ERROR("[BOOT] Failed at RHI initialize");
            return -1;
        }
    }

    LOG_INFO("[BOOT] RHI initialized type=%s", ToString(RHI::Device->GetRHIType()));

    CoreDelegates::PostInitRHIDelegate.Broadcast();

    {
        TRACE_BOOT_SCOPE(TEXT("ShaderCache"));

        if (!FShaderCache::Initialize())
        {
            return -1;
        }

        FShaderBytecodeCache::Initialize();
    }

    {
        TRACE_BOOT_SCOPE(TEXT("TextureFactory"));

        if (!FTextureFactory::Initialize())
        {
            return -1;
        }
    }

    {
        TRACE_BOOT_SCOPE(TEXT("VertexStreamCache"));
        
        if (!FVertexStreamCache::Initialize())
        {
            return -1;
        }
    }

    {
        TRACE_BOOT_SCOPE(TEXT("RenderGraphResourcePool"));

        if (!FRenderGraphResourcePool::Initialize())
        {
            return -1;
        }
    }

    {
        TRACE_BOOT_SCOPE(TEXT("ShaderPrewarm"));
        FShaderCache::Get().PrewarmAsync();
    }

    CoreDelegates::PreInitFinishedDelegate.Broadcast();
    return 0;
}

bool FEngineLoop::CreateApplicationRenderer()
{
    UIRenderer = MakeSharedPtr<FApplicationRenderer>();
    if (!UIRenderer || !UIRenderer->InitializeRHI())
    {
        return false;
    }

    FApplication::Get().SetRenderer(UIRenderer);
    FApplication::Get().SetOnWindowLiveResize(FOnWindowLiveResize::CreateRaw(this, &FEngineLoop::RedrawWindowDuringResize));
    return true;
}

void FEngineLoop::RedrawWindowDuringResize(const TSharedPtr<FWindow>& Window)
{
    if (!UIRenderer || !GSynchronousResize || bIsRedrawingForResize)
    {
        return;
    }

    bIsRedrawingForResize = true;

    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->FinishPreviousFrame();
    }

    UIRenderer->EndFrameAndPresent();
    UIRenderer->RedrawWindow(Window);

    bIsRedrawingForResize = false;
}

int32 FEngineLoop::Init()
{
    IRendererModule* RendererModule = nullptr;

    {
        TRACE_BOOT_SCOPE(TEXT("Init"));

        {
            TRACE_BOOT_SCOPE(TEXT("ImGuiPlugin"));

            IImguiPlugin* ImguiPlugin = FModuleManager::Get().LoadModule<IImguiPlugin>(TEXT("ImGuiPlugin"));
            if (!ImguiPlugin)
            {
                LOG_ERROR("Failed to load ImGuiPlugin");
                return -1;
            }
        }

        {
            TRACE_BOOT_SCOPE(TEXT("ApplicationRenderer"));

            if (!CreateApplicationRenderer())
            {
                LOG_ERROR("[BOOT] Failed at ApplicationRenderer RHI initialize");
                FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("FAILED to create the UI renderer"));
                return -1;
            }
        }

        LOG_INFO("[BOOT] ApplicationRenderer RHI initialized");

        CoreDelegates::PreEngineInitDelegate.Broadcast();

        {
            TRACE_BOOT_SCOPE(TEXT("Engine"));

            if (!FEngine::Initialize())
            {
                LOG_ERROR("Failed to initialize engine");
                return -1;
            }
        }

        LogStartupInformation();

    #if EDITOR_BUILD
        FString RegistryError;
        if (!FEngineRegistry::RegisterRunningEngine(RegistryError))
        {
            LOG_WARNING("Could not register this engine in the per-user engine registry: %s", *RegistryError);
        }
    #endif

        CoreDelegates::PreEngineInitDelegate.Broadcast();

        {
            TRACE_BOOT_SCOPE(TEXT("RendererModule"));

            RendererModule = IRendererModule::Get();
            if (!RendererModule->Initialize())
            {
                LOG_ERROR("[BOOT] Failed at scene renderer initialize");
                FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("FAILED to create Renderer"));
                return -1;
            }
        }

        LOG_INFO("[BOOT] Scene renderer initialized");

        UIRenderer->SetGPUProfiler(&RendererModule->GetGPUProfiler());

        {
            TRACE_BOOT_SCOPE(TEXT("ShaderCompileStats"));
            FShaderCompiler::Get().LogCompileStats();
        }

        CoreDelegates::PreApplicationLoadedDelegate.Broadcast();

        if (IImguiPlugin::IsEnabled())
        {
            TRACE_BOOT_SCOPE(TEXT("ImGuiRHI"));

            if (!IImguiPlugin::Get().InitializeRHI())
            {
                FPlatformApplicationMisc::MessageBox(TEXT("ERROR"), TEXT("FAILED to initialize RHI resources for ImGui"));
                return -1;
            }
        }

        {
            TRACE_BOOT_SCOPE(TEXT("InitPostRenderer"));

            if (!FEngine::Get()->InitPostRenderer())
            {
                return -1;
            }
        }

        {
            TRACE_BOOT_SCOPE(TEXT("EngineStart"));
            
            if (!FEngine::Get()->Start())
            {
                LOG_ERROR("[BOOT] Failed at engine start");
                return -1;
            }
        }
    }

    FBootProfiler::Get().Finish();
    FFrameProfiler::Get().Enable();
    TRACE_EVENT(TEXT("BootFinished"));

    FProfileRun::BeginCapture();

    LOG_INFO("[BOOT] Engine started");
    return 0;
}

void FEngineLoop::Tick()
{
    {
        TRACE_FUNCTION_SCOPE();

        // Run any work that was queued onto the main thread since the last tick.
        {
            TRACE_SCOPE(TEXT("Process Main Thread Tasks"));
            Tasks::ProcessMainThreadTasks();
        }

        {
            TRACE_SCOPE(TEXT("Frame Timer Tick"));
            FrameTimer.Tick();
        }

        const float DeltaTime = static_cast<float>(FrameTimer.GetDeltaTime().AsSeconds());
        FApplication::Get().Tick(DeltaTime);

        // The window-close message is pumped above; once exit is requested the surface may already be gone.
        if (IsEngineExitRequested())
        {
            return;
        }

        IRendererModule* RendererModule = IRendererModule::Get();
        RendererModule->FinishPreviousFrame();

        IGPUProfiler& GPUProfiler = RendererModule->GetGPUProfiler();

        UIRenderer->EndFrameAndPresent();
        {
            TRACE_SCOPE(TEXT("GPU Profiler End Frame"));
            GPUProfiler.EndGPUFrame();
        }

        FApplication::Get().ProcessDeferredEvents();

        {
            TRACE_SCOPE(TEXT("Engine Tick"));
            FEngine::Get()->Tick(DeltaTime);
        }

        {
            TRACE_SCOPE(TEXT("GPU Profiler Begin Frame"));
            GPUProfiler.BeginGPUFrame();
        }

        UIRenderer->BeginFrame();
        UIRenderer->RecordWindows();

        if (IImguiPlugin::IsEnabled())
        {
            TRACE_SCOPE(TEXT("ImGui Draw"));
            IImguiPlugin::Get().Draw(UIRenderer->GetCommandList());
            IImguiPlugin::Get().DrawViewports(UIRenderer->GetCommandList());
        }

        RendererModule->Tick();

        FSceneRenderPacket Packet = FEngine::Get()->BuildRenderPacket();
        RendererModule->KickSceneRender(::Move(Packet));

        {
            TRACE_SCOPE(TEXT("Memory Page Pool Tick"));
            FMemoryPagePool::Get().Tick();
        }

        {
            TRACE_SCOPE(TEXT("Platform Event Pool Tick"));
            FPlatformEventPool::Get().Tick();
        }
    }

    FFrameProfiler::Get().Tick();

    ++FrameCounter;

    if (FrameCounter == 1)
    {
        LOG_INFO("[BOOT] First frame submitted");
    }

    FConsoleManager::Get().ExecuteQueuedCommands(*FOutputDeviceManager::Get());

    {
        TRACE_SCOPE(TEXT("RemoteConsole Tick"));
        FRemoteConsoleServer::Tick();
    }

    FProfileRun::Tick();

    const int32 ExitAfterFrames = CVarExitAfterFrames.GetValue();
    if (ExitAfterFrames > 0 && FrameCounter >= static_cast<uint64>(ExitAfterFrames))
    {
        LOG_INFO("[BOOT] ExitAfterFrames reached count=%llu", static_cast<uint64>(FrameCounter));
        RequestEngineExit(TEXT("Engine.ExitAfterFrames reached"));
    }
}

void FEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    FRemoteConsoleServer::Release();

    if (FApplication::IsInitialized())
    {
        FApplication::Get().SetOnWindowLiveResize(FOnWindowLiveResize());
    }

    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->FinishPreviousFrame();
    }

    if (FRHICommandListExecutor::IsInitialized())
    {
        FRHICommandListExecutor::Get().WaitForGPU();
    }

    if (IRendererModule* RendererModule = IRendererModule::Get())
    {
        RendererModule->Release();
    }

    FEngine::Destroy();

    if (UIRenderer)
    {
        FApplication::Get().SetRenderer(nullptr);
        UIRenderer->ReleaseRHI();
        UIRenderer.Reset();
    }

    if (IImguiPlugin::IsEnabled())
    {
        FModuleManager::Get().UnloadModule(TEXT("ImGuiPlugin"));
    }

    // Release all RHI resources
    FTextureFactory::Release();
    FShaderCache::Release();
    FShaderBytecodeCache::Release();
    FVertexStreamCache::Release();
    FRenderGraphResourcePool::Release();

    // Wait for RHI thread and shutdown RHI Layer
    RHI::Release();

    FShaderCompiler::Destroy();

    FTaskGraph::Release();

    FApplication::Release();

    FThreadManager::Release();

    FConfig::Release();

    CoreDelegates::Shutdown();

    FModuleManager::Shutdown();

    FMemoryPagePool::Get().Flush();
    FPlatformEventPool::Get().Flush();

    if (FPlatformMisc::IsDebuggerPresent())
    {
        GMalloc->DumpAllocations(GDebuggerOutputDevice.Get());
    }
}
