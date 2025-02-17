#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Math/Math.h"
#include "Project/ProjectManager.h"
#include "Application/ApplicationInterface.h"
#include "Application/Widgets/WindowWidget.h"
#include "Application/Widgets/ViewportWidget.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Engine/Engine.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Resources/Material.h"
#include "Engine/Widgets/ConsoleWidget.h"
#include "Engine/Widgets/FrameProfilerWidget.h"
#include "Engine/Widgets/SceneInspectorWidget.h"
#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

#if ENGINE_DEBUG_INPUT
    #include "Engine/Debug/InputDebugInputHandler.h"
#endif

ENGINE_API FEngine* GEngine = nullptr;

static void ExitEngineFunc()
{
    if (GEngine)
    {
        GEngine->Exit();
    }
}
 
static void ToggleFullScreenFunc()
{
    DEBUG_BREAK();

    //if (GEngine && GEngine->EngineWindow)
    //{
    //    EWindowMode WindowMode;// = GEngine->MainWindow->GetStyle();
    //    if (WindowMode == EWindowMode::Fullscreen)
    //    {
    //        WindowMode = EWindowMode::Windowed;
    //    }
    //    else
    //    {
    //        WindowMode = EWindowMode::Fullscreen;
    //    }

    //    GEngine->MainWindow->SetWindowMode(WindowMode);
    //}
}

static FAutoConsoleCommand CVarExit(
    "Engine.Exit",
    "Exits the engine",
    FConsoleCommandDelegate::CreateStatic(&ExitEngineFunc));

static FAutoConsoleCommand CVarToggleFullscreen(
    "Engine.ToggleFullscreen",
    "Toggles fullscreen on the main Viewport",
    FConsoleCommandDelegate::CreateStatic(&ToggleFullScreenFunc));

static TAutoConsoleVariable<int32> CVarViewportWidth(
    "Engine.ViewportWidth",
    "Width of the main window",
    1920,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarViewportHeight(
    "Engine.ViewportHeight",
    "Width of the main window",
    1080,
    EConsoleVariableFlags::Default);

FEngine::FEngine()
    : EngineWindow(nullptr)
    , EngineViewportWidget(nullptr)
    , SceneViewport(nullptr)
    , ConsoleWidget(nullptr)
    , ProfilerWidget(nullptr)
    , InspectorWidget(nullptr)
    , World(nullptr)
    , GameModule(nullptr)
{
}

FEngine::~FEngine()
{
    World = nullptr;
}

bool FEngine::CreateEngineWindow()
{
    FWindowWidget::FInitializer WindowInitializer;
    WindowInitializer.Title      = "Sandbox";
    WindowInitializer.Size.X     = CVarViewportWidth.GetValue();
    WindowInitializer.Size.Y     = CVarViewportHeight.GetValue();
    WindowInitializer.StyleFlags = EWindowStyleFlags::Default;
    
    EngineWindow = CreateWidget<FWindowWidget>(WindowInitializer);

    // Initialize and show the game-window
    FApplicationInterface::Get().CreateWindow(EngineWindow);
    return true;
}

bool FEngine::CreateEngineViewport()
{
    if (!EngineWindow)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "MainWindow is not initialized");
        return false;
    }

    FViewportWidget::FInitializer ViewportInitializer;
    ViewportInitializer.ViewportInterface = nullptr;

    EngineViewportWidget = CreateWidget<FViewportWidget>(ViewportInitializer);
    EngineViewportWidget->SetParentWidget(EngineWindow->AsWeakPtr());

    EngineWindow->SetOnWindowMoved(FOnWindowMoved::CreateRaw(this, &FEngine::OnEngineWindowMoved));
    EngineWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FEngine::OnEngineWindowClosed));
    EngineWindow->SetOnWindowResized(FOnWindowResized::CreateRaw(this, &FEngine::OnEngineWindowResized));
    EngineWindow->SetContent(EngineViewportWidget);
    return true;
}

bool FEngine::CreateSceneViewport()
{
    if (!EngineViewportWidget)
    {
        return false;
    }

    // Create a SceneViewport
    SceneViewport = MakeSharedPtr<FSceneViewport>(EngineViewportWidget);
    if (!SceneViewport->InitializeRHI())
    {
        return false;
    }
    else
    {
        SceneViewport->SetWorld(World);
    }

    EngineViewportWidget->SetViewportInterface(SceneViewport);

    // Make sure we have focus on the new viewport
    FApplicationInterface::Get().SetFocusWidget(EngineViewportWidget);
    return true;
}

void FEngine::OnEngineWindowClosed()
{
    RequestEngineExit("Window Closed");
}

void FEngine::OnEngineWindowMoved(const FIntVector2& /* NewScreenPosition */)
{
    // LOG_INFO("Window Moved x=%d y=%d", NewScreenPosition.x, NewScreenPosition.y);
}

void FEngine::OnEngineWindowResized(const FIntVector2& /* NewScreenSize */)
{
    // LOG_INFO("Window Resized x=%d y=%d", NewScreenSize.x, NewScreenSize.y);
}

bool FEngine::Init()
{
    if (!CreateEngineWindow())
    {
        return false;
    }

    if (!CreateEngineViewport())
    {
        return false;
    }

    if (!FAssetManager::Initialize())
    {
        return false;
    }

#if ENGINE_DEBUG_INPUT
    InputDebugInputHandler = MakeSharedPtr<FInputDebugInputHandler>();
    if (FApplicationInterface::IsInitialized() && InputDebugInputHandler)
    {
        FApplicationInterface::Get().RegisterInputHandler(InputDebugInputHandler);
    }
#endif

    // Create standard textures
    uint8 Pixels[4] = { 255, 255, 255, 255 };

    BaseTexture = FTextureFactory::Get().LoadFromMemory(Pixels, 1, 1, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);
    if (!BaseTexture)
    {
        LOG_WARNING("Failed to create BaseTexture");
    }
    else
    {
        BaseTexture->SetDebugName("BaseTexture");
    }

    Pixels[0] = 127;
    Pixels[1] = 127;
    Pixels[2] = 255;

    BaseNormal = FTextureFactory::Get().LoadFromMemory(Pixels, 1, 1, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);
    if (!BaseNormal)
    {
        LOG_WARNING("Failed to create BaseNormal-Texture");
    }
    else
    {
        BaseNormal->SetDebugName("BaseNormal");
    }

    // Create material sampler (Used for now by all materials)
    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU       = ESamplerMode::Wrap;
    SamplerInfo.AddressV       = ESamplerMode::Wrap;
    SamplerInfo.AddressW       = ESamplerMode::Wrap;
    SamplerInfo.ComparisonFunc = EComparisonFunc::Unknown;
    SamplerInfo.Filter         = ESamplerFilter::Anistrotopic;
    SamplerInfo.MaxAnisotropy  = 16;
    SamplerInfo.MaxLOD         = TNumericLimits<float>::Max();
    SamplerInfo.MinLOD         = 0.0f;
    SamplerInfo.MipLODBias     = 0.0f;

    BaseMaterialSampler = RHICreateSamplerState(SamplerInfo);

    // Base material
    FMaterialInfo MaterialDesc;
    MaterialDesc.Albedo           = FFloatColor::White;
    MaterialDesc.Metallic         = 0.0f;
    MaterialDesc.AmbientOcclusion = 1.0f;
    MaterialDesc.Roughness        = 1.0f;

    BaseMaterial = MakeSharedPtr<FMaterial>(MaterialDesc);
    BaseMaterial->AlbedoMap    = GEngine->BaseTexture;
    BaseMaterial->NormalMap    = GEngine->BaseNormal;
    BaseMaterial->RoughnessMap = GEngine->BaseTexture;
    BaseMaterial->AOMap        = GEngine->BaseTexture;
    BaseMaterial->MetallicMap  = GEngine->BaseTexture;
    BaseMaterial->AlphaMask    = GEngine->BaseTexture;
    BaseMaterial->Initialize();

    // Create a new world
    World = new FWorld();
    if (IRendererModule* Renderer = IRendererModule::Get())
    {
        if (IScene* RendererScene = Renderer->CreateScene(World))
        {
            World->SetSceneInterface(RendererScene);
        }
    }

    // Load Game-Module
    const char* GameModuleName = *FProjectManager::Get().GetProjectModuleName();
    GameModule = FModuleManager::Get().LoadModule<FGameModule>(GameModuleName);
    if (!GameModule)
    {
        LOG_ERROR("Failed to load Game-module, the application may not behave as intended");
        return false;
    }

    if (!GameModule->Init())
    {
        LOG_ERROR("Failed to initialize GameModule");
        return false;
    }
    else
    {
        CoreDelegates::PostGameModuleLoadedDelegate.Broadcast();
    }

    // Create the scene viewport (Contains back-buffer etc.)
    if (!CreateSceneViewport())
    {
        return false;
    }

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().SetMainViewport(EngineViewportWidget);

        ProfilerWidget  = MakeSharedPtr<FFrameProfilerWidget>();
        ConsoleWidget   = MakeSharedPtr<FConsoleWidget>();
        InspectorWidget = MakeSharedPtr<FSceneInspectorWidget>();
    }

    return true;
}

bool FEngine::Start()
{
    if (World)
    {
        World->Start();
    }
    else
    {
        DEBUG_BREAK();
    }

    return true;
}

void FEngine::Tick(float DeltaTime)
{
    TRACE_FUNCTION_SCOPE();

    // At this point in the frame, we can update the scene-viewport so that the camera is prepared for this frame
    if (SceneViewport)
    {
        SceneViewport->Tick();
    }

    // Update the game-module
    GameModule->Tick(DeltaTime);

    // Update the world, update all the actors and components
    if (World)
    {
        World->Tick(DeltaTime);
    }

    // Update the ImGui-plugin
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().Tick(DeltaTime);
    }
}

void FEngine::Release()
{
    if (IImguiPlugin::IsEnabled())
    {
        ProfilerWidget.Reset();
        ConsoleWidget.Reset();
        InspectorWidget.Reset();

        IImguiPlugin::Get().SetMainViewport(nullptr);
    }

#if ENGINE_DEBUG_INPUT
    if (FApplicationInterface::IsInitialized() && InputDebugInputHandler)
    {
        FApplicationInterface::Get().UnregisterInputHandler(InputDebugInputHandler);
        InputDebugInputHandler.Reset();
    }
#endif

    // Destroy the World
    if (World)
    {
        SceneViewport->SetWorld(nullptr);

        if (IRendererModule* Renderer = IRendererModule::Get())
        {
            Renderer->DestroyScene(World->GetSceneInterface());
        }

        delete World;
        World = nullptr;
    }

    // Unload the GameModule
    if (GameModule)
    {
        GameModule->Release();

        const CHAR* GameModuleName = *FProjectManager::Get().GetProjectModuleName();
        FModuleManager::Get().UnloadModule(GameModuleName);
        GameModule = nullptr;
    }

    // Release all assets
    FAssetManager::Release();

    // Release RHI resources
    SceneViewport->ReleaseRHI();

    // Reset widgets
    EngineViewportWidget.Reset();
    EngineWindow.Reset();
}

void FEngine::Exit()
{
    // Empty for now
}
