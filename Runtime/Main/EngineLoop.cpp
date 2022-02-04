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

#include "Application/ApplicationInstance.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"

#include "Renderer/Renderer.h"
#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// LoadCoreModules

bool CEngineLoop::LoadCoreModules()
{
    CModuleManager& ModuleManager = CModuleManager::Get();

    IEngineModule* CoreModule = ModuleManager.LoadEngineModule("Core");
    IEngineModule* CoreApplicationModule = ModuleManager.LoadEngineModule("CoreApplication");
    if (!CoreModule || !CoreApplicationModule)
    {
        return false;
    }

    IEngineModule* InterfaceModule = ModuleManager.LoadEngineModule("Application");
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// PreInitialize

bool CEngineLoop::PreInitialize()
{
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

    if (!LoadCoreModules())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to Load Core-Modules");
        return false;
    }

    // TODO: Use a seperate profiler for booting the engine
    CFrameProfiler::Enable();
    TRACE_FUNCTION_SCOPE();

	const CString ProjectLocation = CString(ENGINE_LOCATION) + CString("/") + CString(PROJECT_NAME);
    if (!CProjectManager::Initialize(PROJECT_NAME, ProjectLocation.CStr()))
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to initialize Project");
        return false;
    }

#if !PRODUCTION_BUILD
    LOG_INFO("ProjectName=" + CString(CProjectManager::GetProjectName()));
    LOG_INFO("ProjectPath=" + CString(CProjectManager::GetProjectPath()));
#endif

    if (!PlatformThreadMisc::Initialize())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to init PlatformThreadMisc");
        return false;
    }

    if (!CApplicationInstance::Make())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    if (!CDispatchQueue::Get().Initialize())
    {
        return false;
    }

    // TODO: Decide this via command line
    ERHIInstanceApi RenderApi =
#if PLATFORM_MACOS
        ERHIInstanceApi::Null;
#else
        ERHIInstanceApi::D3D12;
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
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to create Renderer");
        return false;
    }

    NEngineLoopDelegates::PreApplicationLoadedDelegate.Broadcast();

    GApplicationModule = CModuleManager::Get().LoadEngineModule<CApplicationModule>(CProjectManager::GetProjectModuleName());
    if (!GApplicationModule)
    {
        LOG_WARNING("Application Init failed, may not behave as intended");
    }
    else
    {
        NEngineLoopDelegates::PostApplicationLoadedDelegate.Broadcast();
    }

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

    CApplicationInstance::Get().SetRenderer(InterfaceRenderer);

    // Final thing is to startup the engine
    if (!GEngine->Start())
    {
        return false;
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Tick

void CEngineLoop::Tick(CTimestamp Deltatime)
{
    TRACE_FUNCTION_SCOPE();

    CApplicationInstance::Get().Tick(Deltatime);

    CEngineLoopTicker::Get().Tick(Deltatime);

    GEngine->Tick(Deltatime);

    CFrameProfiler::Get().Tick();

    CGPUProfiler::Get().Tick();

    GRenderer.Tick(*GEngine->Scene);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Release

bool CEngineLoop::Release()
{
    TRACE_FUNCTION_SCOPE();

    CRHICommandQueue::Get().WaitForGPU();

    CGPUProfiler::Release();

    GRenderer.Release();

    if (CApplicationInstance::IsInitialized())
    {
        CApplicationInstance::Get().SetRenderer(nullptr);
    }

    // Release the engine. Protect against failed initialization where the global pointer was never initialized
    if (GEngine)
    {
        GEngine->Release();

        GEngine->Destroy();
        GEngine = nullptr;
    }

    CTextureFactory::Release();

    RHIRelease();

    CDispatchQueue::Get().Release();

    CApplicationInstance::Release();

    PlatformThreadMisc::Release();

    CModuleManager::ReleaseAllLoadedModules();

    SafeRelease(NErrorDevice::GConsoleWindow);

    CModuleManager::Destroy();

    return true;
}
