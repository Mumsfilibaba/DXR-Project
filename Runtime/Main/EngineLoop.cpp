#include "EngineLoop.h"

#include "Engine/Engine.h"
#include "Engine/Project/ProjectManager.h"
#include "Engine/Resources/TextureFactory.h"

#if PROJECT_EDITOR
    #include "EditorEngine.h"
#endif

#include "Core/Modules/ModuleInterface.h"
#include "Core/Modules/ApplicationModule.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Threading/TaskManagerInterface.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/EngineLoopTicker.h"
#include "Core/Misc/OutputDeviceConsole.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Application/ApplicationInterface.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"

#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"

#include "RHI/RHIShaderCompiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FEngineLoop

FEngineLoop::FEngineLoop()
    : FrameTimer()
    , ConsoleWindow(nullptr)
{ }

FEngineLoop::~FEngineLoop()
{ 
    ConsoleWindow = nullptr;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// LoadCoreModules

bool FEngineLoop::LoadCoreModules()
{
    FModuleInterface& ModuleManager = FModuleInterface::Get();

    IModule* CoreModule = ModuleManager.LoadModule("Core");
    if (!CoreModule)
    {
        return false;
    }

    IModule* CoreApplicationModule = ModuleManager.LoadModule("CoreApplication");
    if (!CoreApplicationModule)
    {
        return false;
    }

    IModule* ApplicationModule = ModuleManager.LoadModule("Application");
    if (!ApplicationModule)
    {
        return false;
    }

    IModule* EngineModule = ModuleManager.LoadModule("Engine");
    if (!EngineModule)
    {
        return false;
    }

    IModule* RHIModule = ModuleManager.LoadModule("RHI");
    if (!RHIModule)
    {
        return false;
    }

    IModule* RendererModule = ModuleManager.LoadModule("Renderer");
    if (!RendererModule)
    {
        return false;
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// PreInitialize

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

#if !PRODUCTION_BUILD
    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
#endif

    // TODO: Use a separate profiler for booting the engine
    FFrameProfiler::Enable();
    TRACE_FUNCTION_SCOPE();

	const FString ProjectLocation     = FString(ENGINE_LOCATION) + FString("/") + FString(PROJECT_NAME);
    const FString AssetFolderLocation = FString(ENGINE_LOCATION) + FString("/Assets");
    if (!FProjectManager::Initialize(PROJECT_NAME, ProjectLocation.GetCString(), AssetFolderLocation.GetCString()))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
        return false;
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

    if (!FApplicationInterface::Create())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    if (!FTaskManagerInterface::Get().Initialize())
    {
        return false;
    }

   // Initialize the ShaderCompiler before RHI since RHI might need to compile shaders
    if (!FRHIShaderCompiler::Initialize(FProjectManager::GetAssetPath()))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to Initializer ShaderCompiler");
        return false;
    }
        
    // TODO: Decide this via command line
    ERHIInstanceType RenderApi =
#if PLATFORM_MACOS
        ERHIInstanceType::Metal;
#else
        ERHIInstanceType::D3D12;
#endif
    if (!RHIInitialize(RenderApi))
    {
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Initialize

bool FEngineLoop::Init()
{
    NCoreDelegates::PreEngineInitDelegate.Broadcast();

#if PROJECT_EDITOR
    GEngine = FEditorEngine::Make();
#else
    GEngine = dbg_new FEngine();
#endif
    if (!GEngine->Initialize())
    {
        LOG_ERROR("Failed to initialize engine");
        return false;
    }

    NCoreDelegates::PreEngineInitDelegate.Broadcast();

    if (!GRenderer.Init())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    NCoreDelegates::PreApplicationLoadedDelegate.Broadcast();

    GApplicationModule = FModuleInterface::Get().LoadModule<FApplicationInterfaceModule>(FProjectManager::GetProjectModuleName());
    if (!GApplicationModule)
    {
        LOG_WARNING("Application Init failed, may not behave as intended");
    }
    else
    {
        NCoreDelegates::PostApplicationLoadedDelegate.Broadcast();
    }

    IApplicationRendererModule* InterfaceRendererModule = FModuleInterface::Get().LoadModule<IApplicationRendererModule>("InterfaceRenderer");
    if (!InterfaceRendererModule)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load InterfaceRenderer");
        return false;
    }

    TSharedRef<IApplicationRenderer> InterfaceRenderer = InterfaceRendererModule->CreateRenderer();
    if (!InterfaceRenderer)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create InterfaceRenderer");
        return false;
    }

    FApplicationInterface::Get().SetRenderer(InterfaceRenderer);

    // Final thing is to startup the engine
    if (!GEngine->Start())
    {
        return false;
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Tick

void FEngineLoop::Tick()
{
    TRACE_FUNCTION_SCOPE();

    // Tick the timer
    FrameTimer.Tick();

    FApplicationInterface::Get().Tick(FrameTimer.GetDeltaTime());

    FEngineLoopTicker::Get().Tick(FrameTimer.GetDeltaTime());

    GEngine->Tick(FrameTimer.GetDeltaTime());

    FFrameProfiler::Get().Tick();

    FGPUProfiler::Get().Tick();

    GRenderer.Tick(*GEngine->Scene);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Release

bool FEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    GRHICommandExecutor.WaitForGPU();

    FGPUProfiler::Release();

    GRenderer.Release();

    // Release the Application. Protect against failed initialization where the global pointer was never initialized
    if (FApplicationInterface::IsInitialized())
    {
        FApplicationInterface::Get().SetRenderer(nullptr);
    }

    // Release the Engine. Protect against failed initialization where the global pointer was never initialized
    if (GEngine)
    {
        GEngine->Release();

        GEngine->Destroy();
        GEngine = nullptr;
    }

    FTextureFactory::Release();

    RHIRelease();

    FTaskManagerInterface::Get().Release();

    FApplicationInterface::Release();

    FThreadManager::Release();

    SAFE_DELETE(ConsoleWindow);

    FModuleInterface::Shutdown();

    return true;
}
