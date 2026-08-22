#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Math/Math.h"
#include "Core/Misc/Paths.h"
#include "Application/Application.h"
#include "Application/Elements/Window.h"
#include "Application/Elements/Viewport.h"
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

static FAutoConsoleCommand CCmdExit(
    "Engine.Exit",
    "Exits the engine",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        if (FEngine::IsInitialized())
        {
            FEngine::Get()->Exit();
        }
    }));

static FAutoConsoleCommand CCmdToggleFullscreen(
    "Engine.ToggleFullscreen",
    "Toggles fullscreen on the main Viewport",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        DEBUG_BREAK();
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


FEngine* FEngine::Engine = nullptr;

bool FEngine::Initialize()
{
    TUniquePtr<FEngine> LocalEngine;
    
#if EDITOR_BUILD
    LocalEngine = MakeUniquePtr<FEditorEngine>();
#else
    LocalEngine = MakeUniquePtr<FRuntimeEngine>();
#endif

    // Set the global engine pointer since it is used inside functions called by FEngine::Init
    Engine = LocalEngine.Get();

    if (!LocalEngine->Init())
    {
        Engine = nullptr;
        return false;
    }
    else
    {
        Engine = LocalEngine.Release();
        return true;
    }
}

void FEngine::Destroy()
{
    if (Engine)
    {
        Engine->Release();

        delete Engine;
        Engine = nullptr;
    }
}

FEngine::FEngine()
    : World(nullptr)
    , GameModule(nullptr)
    , EngineWindow(nullptr)
    , EngineViewport(nullptr)
    , SceneViewport(nullptr)
{
}

FEngine::~FEngine()
{
    World = nullptr;
}

bool FEngine::CreateEngineWindow()
{
    FWindow::FDesc WindowDesc;
    WindowDesc.Title      = "Sandbox";
    WindowDesc.Size.X     = CVarViewportWidth.GetValue();
    WindowDesc.Size.Y     = CVarViewportHeight.GetValue();
    WindowDesc.StyleFlags = EWindowStyleFlags::Default;

#if EDITOR_BUILD
    WindowDesc.StyleFlags |= EWindowStyleFlags::CustomTitleBar;
#endif

    EngineWindow = FWindow::Create(WindowDesc);

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

    FViewport::FDesc ViewportDesc;
    ViewportDesc.ViewportInterface = nullptr;

    EngineViewport = FViewport::Create(ViewportDesc);

    EngineWindow->SetOnWindowMoved(FOnWindowMoved::CreateRaw(this, &FEngine::OnEngineWindowMoved));
    EngineWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FEngine::OnEngineWindowClosed));
    EngineWindow->SetOnWindowResized(FOnWindowResized::CreateRaw(this, &FEngine::OnEngineWindowResized));
    EngineWindow->SetContent(EngineViewport);
    return true;
}

bool FEngine::CreateSceneViewport()
{
    if (!EngineViewport)
    {
        return false;
    }

    // Create a SceneViewport
    SceneViewport = MakeSharedPtr<FSceneViewport>(EngineViewport);
    if (!SceneViewport->InitializeRHI())
    {
        return false;
    }
    else
    {
        SceneViewport->SetWorld(World);
    }

    EngineViewport->SetViewportInterface(SceneViewport);

    // Communicate the render resolution to the renderer
    FRHISwapChainRef SwapChain = SceneViewport->GetRHISwapChain();
    RenderSettings::ChangeRenderResolution(SwapChain->GetDesc().Width, SwapChain->GetDesc().Height);

    return true;
}

void FEngine::OnEngineWindowClosed()
{
    RequestEngineExit("Window Closed");
}

void FEngine::OnEngineWindowMoved(const IntVector2& /* NewScreenPosition */)
{
}

void FEngine::OnEngineWindowResized(const IntVector2& NewScreenSize)
{
    IRendererModule* RendererModule = IRendererModule::Get();
    RendererModule->ResizeSwapChain(SceneViewport->GetRHISwapChain(), NewScreenSize.X, NewScreenSize.Y);

#ifndef EDITOR_BUILD
    RenderSettings::ChangeRenderResolution(NewScreenSize.X, NewScreenSize.Y);
#endif
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

    BaseMaterialSampler = RHI::CreateSamplerState(SamplerDesc);

    // Base material
    FMaterialInfo MaterialDesc;
    MaterialDesc.Albedo           = FFloatColor::White;
    MaterialDesc.Metallic         = 0.0f;
    MaterialDesc.AmbientOcclusion = 1.0f;
    MaterialDesc.Roughness        = 1.0f;

    BaseMaterial = MakeSharedPtr<FMaterial>(MaterialDesc);
    BaseMaterial->SetTexture(EMaterialTextureSlot::BaseColor, BaseTexture);
    BaseMaterial->SetTexture(EMaterialTextureSlot::Normal, BaseNormal);
    BaseMaterial->SetTexture(EMaterialTextureSlot::MaskA, BaseTexture);
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
    const String GameModuleName = Paths::GetProjectModuleName();
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
        IImguiPlugin::Get().SetMainViewport(EngineViewport);
    }

    return true;
}

bool FEngine::Start()
{
    if (!World)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

bool FEngine::StartPlay()
{
    if (!World || !World->IsEditing())
    {
        return false;
    }

    World->BeginPlay();

    if (SceneViewport)
    {
        SceneViewport->CaptureMouse();
    }

    return true;
}

void FEngine::StopPlay()
{
    if (SceneViewport)
    {
        SceneViewport->ReleaseMouse();
    }

    if (World)
    {
        World->EndPlay();
    }
}

void FEngine::TogglePause()
{
    if (World && !World->IsEditing())
    {
        World->SetPaused(!World->IsPaused());
    }
}

void FEngine::Tick(float DeltaTime)
{
    TRACE_FUNCTION_SCOPE();

    if (SceneViewport)
    {
        SceneViewport->Tick();
    }

    // The game module is game code, so it is held back until the world is actually running
    if (IsPlaying())
    {
        GameModule->Tick(DeltaTime);
    }

    if (World)
    {
        World->Tick(DeltaTime);
    }

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().Tick(DeltaTime);
    }
}

FSceneRenderPacket FEngine::BuildRenderPacket()
{
    FSceneRenderPacket Packet;
    if (SceneViewport)
    {
        Packet.SwapChain = SceneViewport->GetRHISwapChain();
    }

    return Packet;
}

void FEngine::Release()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().SetMainViewport(nullptr);
    }

    // Destroy the World
    if (World)
    {
        SceneViewport->SetWorld(nullptr);

        if (IRendererModule* Renderer = IRendererModule::Get())
        {
            Renderer->DestroyScene(World->GetSceneInterface());
            World->ClearSceneInterface();
        }

        delete World;
        World = nullptr;
    }

    // Unload the GameModule
    if (GameModule)
    {
        GameModule->Release();

        const CHAR* GameModuleName = *Paths::GetProjectModuleName();
        FModuleManager::Get().UnloadModule(GameModuleName);
        GameModule = nullptr;
    }

    // Release all assets
    FAssetManager::Release();

    // Release RHI resources
    SceneViewport->ReleaseRHI();

    // Reset widgets
    EngineViewport.Reset();
    EngineWindow.Reset();
}
