#include "Engine.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Modules/ModuleManager.h"
#include "Project/ProjectManager.h"
#include "Application/Application.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetLoaders/MeshImporter.h"
#include "Engine/Resources/Material.h"
#include "Engine/Widgets/ConsoleWidget.h"
#include "Engine/Widgets/FrameProfilerWidget.h"
#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/Interfaces/IRendererModule.h"

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
    if (GEngine && GEngine->MainWindow)
    {
        EWindowMode WindowMode;// = GEngine->MainWindow->GetStyle();
        if (WindowMode == EWindowMode::Fullscreen)
        {
            WindowMode = EWindowMode::Windowed;
        }
        else
        {
            WindowMode = EWindowMode::Fullscreen;
        }

        //GEngine->MainWindow->SetWindowMode(WindowMode);
    }
}

static FAutoConsoleCommand GExit(
    "Engine.Exit",
    "Exits the engine",
    FConsoleCommandDelegate::CreateStatic(&ExitEngineFunc));

static FAutoConsoleCommand GToggleFullscreen(
    "MainViewport.ToggleFullscreen",
    "Toggles fullscreen on the main Viewport",
    FConsoleCommandDelegate::CreateStatic(&ToggleFullScreenFunc));

FEngine::FEngine()
    : World(nullptr)
{
}

FEngine::~FEngine()
{
    World = nullptr;
}

void FEngine::CreateMainWindow()
{
    // TODO: This should be loaded from a config file
    FGenericWindowInitializer WindowInitializer;
    WindowInitializer.Title  = "Sandbox";
    WindowInitializer.Width  = 2560;
    WindowInitializer.Height = 1440;
    
    TSharedRef<FGenericWindow> Window = FApplication::Get().CreateWindow(WindowInitializer);
    if (!Window)
    {
        DEBUG_BREAK();
        return;
    }
    
    MainWindow = Window;
}

bool FEngine::CreateMainViewport()
{
    if (!MainWindow)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "MainWindow is not initialized");
        return false;
    }

    TSharedPtr<FViewport> Viewport = MakeShared<FViewport>();
    if (!Viewport)
    {
        DEBUG_BREAK();
        return false;
    }
    
    FViewportInitializer ViewportInitializer;
    ViewportInitializer.Window = MainWindow.Get();
    ViewportInitializer.Width  = MainWindow->GetWidth();
    ViewportInitializer.Height = MainWindow->GetHeight();
    
    if (!Viewport->InitializeRHI(ViewportInitializer))
    {
        DEBUG_BREAK();
        return false;
    }

    // Set the main-viewport
    MainViewport = Viewport;

    // NOTE: We need to show the window before creating the viewport, since we could ask for a bigger window than what we actually can have Now we show the window
    MainWindow->Show();
    
    FApplication::Get().RegisterMainViewport(MainViewport);
    return true;
}

bool FEngine::Init()
{
    CreateMainWindow();

    if (!CreateMainViewport())
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
    FRHISamplerStateDesc SamplerCreateInfo;
    SamplerCreateInfo.AddressU       = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressV       = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressW       = ESamplerMode::Wrap;
    SamplerCreateInfo.ComparisonFunc = EComparisonFunc::Never;
    SamplerCreateInfo.Filter         = ESamplerFilter::Anistrotopic;
    SamplerCreateInfo.MaxAnisotropy  = 16;
    SamplerCreateInfo.MaxLOD         = TNumericLimits<float>::Max();
    SamplerCreateInfo.MinLOD         = TNumericLimits<float>::Lowest();
    SamplerCreateInfo.MipLODBias     = 0.0f;

    BaseMaterialSampler = RHICreateSamplerState(SamplerCreateInfo);

    // Base material
    FMaterialCreateInfo MaterialDesc;
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

    // Create a SceneViewport
    SceneViewport = MakeShared<FSceneViewport>(MainViewport);
    SceneViewport->SetWorld(World);
    MainViewport->SetViewportInterface(SceneViewport);

    // Create Widgets
    if (FApplication::IsInitialized())
    {
        ProfilerWidget = MakeShared<FFrameProfilerWidget>();
        FApplication::Get().AddWidget(ProfilerWidget);

        ConsoleWidget = MakeShared<FConsoleWidget>();
        FApplication::Get().AddWidget(ConsoleWidget);
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

void FEngine::Tick(FTimespan DeltaTime)
{
    TRACE_FUNCTION_SCOPE();

    if (World)
    {
        World->Tick(DeltaTime);
    }
}

void FEngine::Release()
{
    if (FApplication::IsInitialized())
    {
        FApplication::Get().RemoveWidget(ProfilerWidget);
        ProfilerWidget.Reset();

        FApplication::Get().RemoveWidget(ConsoleWidget);
        ConsoleWidget.Reset();
    }

    if (World)
    {
        SceneViewport->SetWorld(nullptr);

        if (IRendererModule* Renderer = IRendererModule::Get())
        {
            Renderer->DestroyScene(World->GetSceneInterface());
        }

        delete World;
    }

    FAssetManager::Release();

    FMeshImporter::Release();

    MainViewport->ReleaseRHI();
}

void FEngine::Exit()
{
    RequestEngineExit("Normal Exit");
}
