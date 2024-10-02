#include "Engine.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Math/Math.h"
#include "Project/ProjectManager.h"
#include "Application/Application.h"
#include "Application/Widgets/Window.h"
#include "Application/Widgets/Viewport.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetLoaders/MeshImporter.h"
#include "Engine/Resources/Material.h"
#include "Engine/Widgets/ConsoleWidget.h"
#include "Engine/Widgets/FrameProfilerWidget.h"
#include "Engine/Widgets/InspectorWidget.h"
#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

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
    FWindow::FInitializer WindowInitializer;
    WindowInitializer.Title  = "Sandbox";
    WindowInitializer.Size.x = CVarViewportWidth.GetValue();
    WindowInitializer.Size.y = CVarViewportHeight.GetValue();

    EngineWindow = CreateWidget<FWindow>(WindowInitializer);

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

    FViewport::FInitializer ViewportInitializer;
    ViewportInitializer.ViewportInterface = nullptr;

    EngineViewportWidget = CreateWidget<FViewport>(ViewportInitializer);
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
    SceneViewport = MakeShared<FSceneViewport>(EngineViewportWidget);
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

void FEngine::OnEngineWindowMoved(const FIntVector2& NewScreenPosition)
{
    // LOG_INFO("Window Moved x=%d y=%d", NewScreenPosition.x, NewScreenPosition.y);
}

void FEngine::OnEngineWindowResized(const FIntVector2& NewScreenSize)
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

    if (!FMeshImporter::Initialize())
    {
        return false;
    }

    // Create standard textures
    uint8 Pixels[4] = { 255, 255, 255, 255 };

    BaseTexture = FTextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
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

    BaseNormal = FTextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
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
    SamplerInfo.ComparisonFunc = EComparisonFunc::Never;
    SamplerInfo.Filter         = ESamplerFilter::Anistrotopic;
    SamplerInfo.MaxAnisotropy  = 16;
    SamplerInfo.MaxLOD         = TNumericLimits<float>::Max();
    SamplerInfo.MinLOD         = TNumericLimits<float>::Lowest();
    SamplerInfo.MipLODBias     = 0.0f;

    BaseMaterialSampler = RHICreateSamplerState(SamplerInfo);

    // Base material
    FMaterialInfo MaterialDesc;
    MaterialDesc.AmbientOcclusion = 1.0f;
    MaterialDesc.Metallic         = 0.0f;
    MaterialDesc.Roughness        = 1.0f;
    MaterialDesc.Albedo           = FVector3(1.0f);

    BaseMaterial = MakeShared<FMaterial>(MaterialDesc);
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
    const CHAR* GameModuleName = FProjectManager::Get().GetProjectModuleName().GetCString();
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

        ProfilerWidget  = MakeShared<FFrameProfilerWidget>();
        ConsoleWidget   = MakeShared<FConsoleWidget>();
        InspectorWidget = MakeShared<FInspectorWidget>();
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

    GameModule->Tick(DeltaTime);

    if (World)
    {
        World->Tick(DeltaTime);
    }

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

        const CHAR* GameModuleName = FProjectManager::Get().GetProjectModuleName().GetCString();
        FModuleManager::Get().UnloadModule(GameModuleName);
        GameModule = nullptr;
    }

    FAssetManager::Release();

    FMeshImporter::Release();

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
