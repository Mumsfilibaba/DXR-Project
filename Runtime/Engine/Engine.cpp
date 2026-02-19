#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Math/Math.h"
#include "Core/Misc/Paths.h"
#include "Application/Application.h"
#include "Application/Widgets/WindowWidget.h"
#include "Application/Widgets/ViewportWidget.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#if EDITOR_BUILD
#include "Engine/EditorEngine.h"
#else
#include "Engine/RuntimeEngine.h"
#endif
#include "Engine/Assets/AssetManager.h"
#include "Engine/Resources/Material.h"
#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/RenderSettings.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

#if ENGINE_DEBUG_INPUT
#include "Engine/Debug/InputDebugInputHandler.h"
#endif

static void ExitEngineFunc()
{
    if (FEngine::IsInitialized())
    {
        FEngine::Get()->Exit();
    }
}
 
static void ToggleFullScreenFunc()
{
    DEBUG_BREAK();

    //if (FEngine::IsInitialized() && FEngine::Get()->EngineWindow)
    //{
    //    EWindowMode WindowMode;// = FEngine::Get()->MainWindow->GetStyle();
    //    if (WindowMode == EWindowMode::Fullscreen)
    //    {
    //        WindowMode = EWindowMode::Windowed;
    //    }
    //    else
    //    {
    //        WindowMode = EWindowMode::Fullscreen;
    //    }

    //    FEngine::Get()->MainWindow->SetWindowMode(WindowMode);
    //}
}

static FAutoConsoleCommand CVarExit(
    "Engine.Exit",
    "Exits the engine",
    FConsoleCommandDelegate::CreateLambda([](FStringView)
    {
        ExitEngineFunc();
    }));

static FAutoConsoleCommand CVarToggleFullscreen(
    "Engine.ToggleFullscreen",
    "Toggles fullscreen on the main Viewport",
    FConsoleCommandDelegate::CreateLambda([](FStringView)
    {
        ToggleFullScreenFunc();
    }));

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


FEngine* FEngine::GEngine = nullptr;

bool FEngine::Create()
{
    TUniquePtr<FEngine> LocalEngine;
    
#if EDITOR_BUILD
    LocalEngine = MakeUniquePtr<FEditorEngine>();
#else
    LocalEngine = MakeUniquePtr<FRuntimeEngine>();
#endif

    // Set the global engine pointer since it is used inside functions called by FEngine::Init
    GEngine = LocalEngine.Get();

    if (!LocalEngine->Init())
    {
        GEngine = nullptr;
        return false;
    }
    else
    {
        GEngine = LocalEngine.Release();
        return true;
    }
}

void FEngine::Destroy()
{
    if (GEngine)
    {
        GEngine->Release();

        delete GEngine;
        GEngine = nullptr;
    }
}

FEngine::FEngine()
    : EngineWindow(nullptr)
    , EngineViewportWidget(nullptr)
    , SceneViewport(nullptr)
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
    FApplication::Get().CreateWindow(EngineWindow);
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

void FEngine::OnEngineWindowResized(const FIntVector2& NewScreenSize)
{
    // LOG_INFO("Window Resized x=%d y=%d", NewScreenSize.x, NewScreenSize.y);

    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->ResizeSwapChain(SceneViewport->GetRHISwapChain(), NewScreenSize.X, NewScreenSize.Y);
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
    if (FApplication::IsInitialized() && InputDebugInputHandler)
    {
        FApplication::Get().RegisterInputHandler(InputDebugInputHandler);
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
    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU       = ESamplerMode::Wrap;
    SamplerDesc.AddressV       = ESamplerMode::Wrap;
    SamplerDesc.AddressW       = ESamplerMode::Wrap;
    SamplerDesc.ComparisonFunc = EComparisonFunc::Unknown;
    SamplerDesc.Filter         = ESamplerFilter::Anistrotopic;
    SamplerDesc.MaxAnisotropy  = 16;
    SamplerDesc.MaxLOD         = TNumericLimits<float>::Max();
    SamplerDesc.MinLOD         = 0.0f;
    SamplerDesc.MipLODBias     = 0.0f;

    BaseMaterialSampler = FRHI::Get()->CreateSamplerState(SamplerDesc);

    // Base material
    FMaterialInfo MaterialDesc;
    MaterialDesc.Albedo           = FFloatColor::White;
    MaterialDesc.Metallic         = 0.0f;
    MaterialDesc.AmbientOcclusion = 1.0f;
    MaterialDesc.Roughness        = 1.0f;

    BaseMaterial = MakeSharedPtr<FMaterial>(MaterialDesc);
    BaseMaterial->AlbedoMap    = BaseTexture;
    BaseMaterial->NormalMap    = BaseNormal;
    BaseMaterial->RoughnessMap = BaseTexture;
    BaseMaterial->AOMap        = BaseTexture;
    BaseMaterial->MetallicMap  = BaseTexture;
    BaseMaterial->AlphaMask    = BaseTexture;
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
    const FString GameModuleName = FPaths::GetProjectModuleName();
    GameModule = FModuleManager::Get().LoadModule<FGameModule>(*GameModuleName);

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

    // Create the scene viewport
    if (!CreateSceneViewport())
    {
        return false;
    }

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().SetMainViewport(EngineViewportWidget);
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

    if (SceneViewport)
    {
        SceneViewport->Tick();
    }

    GameModule->Tick(DeltaTime);

    if (World)
    {
        World->Tick(DeltaTime);
    }

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().Tick(DeltaTime);
    }

    // Prepare the swapchain
    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->PrepareSwapChain(SceneViewport->GetRHISwapChain());
}

void FEngine::RenderFrame()
{
    TRACE_FUNCTION_SCOPE();

    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->RenderUI();
    RendererModule->PresentSwapChain(SceneViewport->GetRHISwapChain());
}

void FEngine::Release()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().SetMainViewport(nullptr);
    }

#if ENGINE_DEBUG_INPUT
    if (FApplication::IsInitialized() && InputDebugInputHandler)
    {
        FApplication::Get().UnregisterInputHandler(InputDebugInputHandler);
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

        const CHAR* GameModuleName = *FPaths::GetProjectModuleName();
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
