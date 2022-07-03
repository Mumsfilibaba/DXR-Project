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
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Canvas/CanvasApplication.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"

#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"

#include "RHI/RHIShaderCompiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// LoadCoreModules

bool CEngineLoop::LoadCoreModules()
{
    FModuleManager& ModuleManager = FModuleManager::Get();

    IEngineModule* CoreModule            = ModuleManager.LoadEngineModule("Core");
    IEngineModule* CoreApplicationModule = ModuleManager.LoadEngineModule("CoreApplication");
    if (!CoreModule || !CoreApplicationModule)
    {
        return false;
    }

    IEngineModule* CanvasModule = ModuleManager.LoadEngineModule("Canvas");
    if (!CanvasModule)
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// PreInitialize

bool CEngineLoop::PreInitialize()
{
    NErrorDevice::GConsoleWindow = FPlatformApplicationMisc::CreateConsoleWindow();
    if (!NErrorDevice::GConsoleWindow)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize ConsoleWindow");
        return false;
    }
    else
    {
        NErrorDevice::GConsoleWindow->Show(true);
        NErrorDevice::GConsoleWindow->SetTitle(FString(PROJECT_NAME) + ": Error Console");
    }

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
    if (!CProjectManager::Initialize(PROJECT_NAME, ProjectLocation.CStr(), AssetFolderLocation.CStr()))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
        return false;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("ProjectName=%s", CProjectManager::GetProjectName());
    LOG_INFO("ProjectPath=%s", CProjectManager::GetProjectPath());
#endif

    if (!FThreadManager::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ThreadManager");
        return false;
    }

    if (!CCanvasApplication::CreateApplication())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    if (!FAsyncTaskManager::Get().Initialize())
    {
        return false;
    }

   // Initialize the shadercompiler before RHI since RHI might need to compile shaders
    if (!FRHIShaderCompiler::Initialize(CProjectManager::GetAssetPath()))
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

    if (!CGPUProfiler::Init())
    {
        LOG_ERROR("CGPUProfiler failed to be initialized");
    }

    if (!CTextureFactory::Init())
    {
        return false;
    }

    NEngineLoopDelegates::PreInitFinishedDelegate.Broadcast();

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Initialize

bool CEngineLoop::Initialize()
{
    NEngineLoopDelegates::PreEngineInitDelegate.Broadcast();

#if PROJECT_EDITOR
    GEngine = CEditorEngine::Make();
#else
    GEngine = CEngine::Make();
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

    GApplicationModule = FModuleManager::Get().LoadEngineModule<FApplicationModule>(CProjectManager::GetProjectModuleName());
    if (!GApplicationModule)
    {
        LOG_WARNING("Application Init failed, may not behave as intended");
    }
    else
    {
        NEngineLoopDelegates::PostApplicationLoadedDelegate.Broadcast();
    }

    IInterfaceRendererModule* InterfaceRendererModule = FModuleManager::Get().LoadEngineModule<IInterfaceRendererModule>("InterfaceRenderer");
    if (!InterfaceRendererModule)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load InterfaceRenderer");
        return false;
    }

    TSharedRef<ICanvasRenderer> InterfaceRenderer = InterfaceRendererModule->CreateRenderer();
    if (!InterfaceRenderer)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create InterfaceRenderer");
        return false;
    }

    CCanvasApplication::Get().SetRenderer(InterfaceRenderer);

    // Final thing is to startup the engine
    if (!GEngine->Start())
    {
        return false;
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Tick

void CEngineLoop::Tick(FTimestamp Deltatime)
{
    TRACE_FUNCTION_SCOPE();

    CCanvasApplication::Get().Tick(Deltatime);

    FEngineLoopTicker::Get().Tick(Deltatime);

    GEngine->Tick(Deltatime);

    FFrameProfiler::Get().Tick();

    CGPUProfiler::Get().Tick();

    GRenderer.Tick(*GEngine->Scene);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Release

bool CEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    FRHICommandQueue::Get().WaitForGPU();

    CGPUProfiler::Release();

    GRenderer.Release();

    // Release the Application. Protect against failed initialization where the global pointer was never initialized
    if (CCanvasApplication::IsInitialized())
    {
        CCanvasApplication::Get().SetRenderer(nullptr);
    }

    // Release the Engine. Protect against failed initialization where the global pointer was never initialized
    if (GEngine)
    {
        GEngine->Release();

        GEngine->Destroy();
        GEngine = nullptr;
    }

    CTextureFactory::Release();

    RHIRelease();

    FAsyncTaskManager::Get().Release();

    CCanvasApplication::Release();

    FThreadManager::Release();

    FModuleManager::ReleaseAllLoadedModules();

    SafeRelease(NErrorDevice::GConsoleWindow);

    FModuleManager::Destroy();

    return true;
}
