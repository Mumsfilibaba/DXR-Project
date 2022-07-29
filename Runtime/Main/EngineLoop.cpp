#include "EngineLoop.h"

#include "Engine/Engine.h"
#include "Engine/Project/ProjectManager.h"
#include "Engine/Resources/TextureFactory.h"

#if PROJECT_EDITOR
    #include "EditorEngine.h"
#endif

#include "Core/Modules/ModuleManager.h"
#include "Core/Modules/ApplicationModule.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Threading/AsyncTaskManager.h"
#include "Core/Misc/EngineLoopDelegates.h"
#include "Core/Misc/EngineLoopTicker.h"
#include "Core/Misc/OutputDeviceConsole.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Canvas/Application.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"

#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"

#include "RHI/RHIShaderCompiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// LoadCoreModules

bool FEngineLoop::LoadCoreModules()
{
    FModuleManager& ModuleManager = FModuleManager::Get();

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

    IModule* CanvasModule = ModuleManager.LoadModule("Canvas");
    if (!CanvasModule)
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

FOutputDeviceConsole* GConsoleWindow = 0;

bool FEngineLoop::PreInitialize()
{
    // Create the console window
    GConsoleWindow = FPlatformApplicationMisc::CreateOutputDeviceConsole();
    if (GConsoleWindow)
    {
        FOutputDeviceLogger::Get()->AddOutputDevice(GConsoleWindow);
        
        GConsoleWindow->Show(true);
        GConsoleWindow->SetTitle(FString(PROJECT_NAME) + ": Error Console");
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

    LOG_INFO("IsDebuggerAttached=%s", FPlatformMisc::IsDebuggerPresent() ? "true" : "false");
    
    // TODO: Use a separate profiler for booting the engine
    FFrameProfiler::Enable();
    TRACE_FUNCTION_SCOPE();

	const FString ProjectLocation     = FString(ENGINE_LOCATION) + FString("/") + FString(PROJECT_NAME);
    const FString AssetFolderLocation = FString(ENGINE_LOCATION) + FString("/Assets");
    if (!FProjectManager::Initialize(PROJECT_NAME, ProjectLocation.CStr(), AssetFolderLocation.CStr()))
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

    if (!FApplication::CreateApplication())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    if (!FAsyncTaskManager::Get().Initialize())
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

    NEngineLoopDelegates::PostInitRHIDelegate.Broadcast();

    if (!FGPUProfiler::Init())
    {
        LOG_ERROR("FGPUProfiler failed to be initialized");
    }

    if (!FTextureFactory::Init())
    {
        return false;
    }

    NEngineLoopDelegates::PreInitFinishedDelegate.Broadcast();

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Initialize

bool FEngineLoop::Initialize()
{
    NEngineLoopDelegates::PreEngineInitDelegate.Broadcast();

#if PROJECT_EDITOR
    GEngine = FEditorEngine::Make();
#else
    GEngine = FEngine::CreateEngine();
#endif
    if (!GEngine->Initialize())
    {
        LOG_ERROR("Failed to initialize engine");
        return false;
    }

    NEngineLoopDelegates::PreEngineInitDelegate.Broadcast();

    if (!GRenderer.Init())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    NEngineLoopDelegates::PreApplicationLoadedDelegate.Broadcast();

    GApplicationModule = FModuleManager::Get().LoadModule<FApplicationModule>(FProjectManager::GetProjectModuleName());
    if (!GApplicationModule)
    {
        LOG_WARNING("Application Init failed, may not behave as intended");
    }
    else
    {
        NEngineLoopDelegates::PostApplicationLoadedDelegate.Broadcast();
    }

    IApplicationRendererModule* InterfaceRendererModule = FModuleManager::Get().LoadModule<IApplicationRendererModule>("InterfaceRenderer");
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

    FApplication::Get().SetRenderer(InterfaceRenderer);

    // Final thing is to startup the engine
    if (!GEngine->Start())
    {
        return false;
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Tick

void FEngineLoop::Tick(FTimestamp Deltatime)
{
    TRACE_FUNCTION_SCOPE();

    FApplication::Get().Tick(Deltatime);

    FEngineLoopTicker::Get().Tick(Deltatime);

    GEngine->Tick(Deltatime);

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
    if (FApplication::IsInitialized())
    {
        FApplication::Get().SetRenderer(nullptr);
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

    FAsyncTaskManager::Get().Release();

    FApplication::Release();

    FThreadManager::Release();

    FModuleManager::ReleaseAllLoadedModules();

    SafeDelete(GConsoleWindow);

    FModuleManager::Destroy();

    return true;
}
