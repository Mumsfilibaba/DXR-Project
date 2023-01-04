#include "Engine.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Modules/ModuleManager.h"
#include "Application/ApplicationInterface.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetLoaders/MeshImporter.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/TextureFactory.h"
#include "Engine/InterfaceWindows/GameConsoleWindow.h"
#include "Engine/InterfaceWindows/FrameProfilerWindow.h"
#include "Engine/Project/ProjectManager.h"
#include "RHI/RHIInterface.h"

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
    FGenericWindowRef MainWindow = FApplication::Get().GetMainViewport();
    if (MainWindow)
    {
        MainWindow->ToggleFullscreen();
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


bool FEngine::Initialize()
{
    if (!FAssetManager::Initialize())
    {
        return false;
    }

    if (!FMeshImporter::Initialize())
    {
        return false;
    }

    FApplication& Application = FApplication::Get();
    
    // Initialize the Main Viewport
    FViewportInitializer ViewportInitializer(1920, 1080);

    MainViewport = new FSceneViewport(ViewportInitializer);
    if (MainViewport && MainViewport->Create())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Main Viewport");
        return false;
    }
    
    Application.RegisterMainViewport(MainViewport);

    // Register the user
    TSharedPtr<ICursor> Cursor = Application.GetCursor();
    User = FUser::Make(0, Cursor);
    if (!User)
    {
        return false;
    }

    Application.RegisterUser(User);

    // Create standard textures
    uint8 Pixels[4] = { 255, 255, 255, 255 };

    BaseTexture = FTextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!BaseTexture)
    {
        LOG_WARNING("Failed to create BaseTexture");
    }
    else
    {
        BaseTexture->SetName("BaseTexture");
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
        BaseNormal->SetName("BaseNormal");
    }

    /* Create material sampler (Used for now by all materials) */
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

    /* Base material */
    FMaterialDesc MaterialDesc;
    MaterialDesc.AO           = 1.0f;
    MaterialDesc.Metallic     = 0.0f;
    MaterialDesc.Roughness    = 1.0f;
    MaterialDesc.EnableHeight = 0;
    MaterialDesc.Albedo       = FVector3(1.0f);

    BaseMaterial = MakeShared<FMaterial>(MaterialDesc);
    BaseMaterial->AlbedoMap    = GEngine->BaseTexture;
    BaseMaterial->NormalMap    = GEngine->BaseNormal;
    BaseMaterial->RoughnessMap = GEngine->BaseTexture;
    BaseMaterial->AOMap        = GEngine->BaseTexture;
    BaseMaterial->MetallicMap  = GEngine->BaseTexture;
    BaseMaterial->AlphaMask    = GEngine->BaseTexture;
    BaseMaterial->Initialize();

    /* Create the start scene */
    Scene = MakeShared<FScene>();

    /* Create windows */
    TSharedRef<FFrameProfilerWindow> ProfilerWindow = FFrameProfilerWindow::Create();
    Application.AddWindow(ProfilerWindow);

    TSharedRef<FGameConsoleWindow> ConsoleWindow = FGameConsoleWindow::Make();
    Application.AddWindow(ConsoleWindow);

    return true;
}

bool FEngine::Start()
{
    Scene->Start();
    return true;
}

void FEngine::Tick(FTimespan DeltaTime)
{
    TRACE_FUNCTION_SCOPE();

    if (Scene)
    {
        Scene->Tick(DeltaTime);
    }
}

bool FEngine::Release()
{
    return true;
}

void FEngine::Exit()
{
    FPlatformApplicationMisc::RequestExit(0);
}

void FEngine::Destroy()
{
    FAssetManager::Release();

    FMeshImporter::Release();

    delete this;
}
