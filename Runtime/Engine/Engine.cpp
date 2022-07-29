#include "Engine.h"

#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Modules/ModuleManager.h"

#include "Canvas/Application.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Engine/Resources/Material.h"
#include "Engine/Resources/TextureFactory.h"
#include "Engine/InterfaceWindows/GameConsoleWindow.h"
#include "Engine/InterfaceWindows/FrameProfilerWindow.h"
#include "Engine/Project/ProjectManager.h"

#include "RHI/RHICoreInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

ENGINE_API FEngine* GEngine;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleCommands

FAutoConsoleCommand GExit("Engine.Exit");
FAutoConsoleCommand GToggleFullscreen("MainViewport.ToggleFullscreen");

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FEngine

FEngine::FEngine()
{
	GExit.GetDelgate().AddRaw(this, &FEngine::Exit);
}

bool FEngine::Initialize()
{
    const uint32 Style =
        WindowStyleFlag_Titled |
        WindowStyleFlag_Closable |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    FApplication& Application = FApplication::Get();

    const uint32 WindowWidth  = 1920;
    const uint32 WindowHeight = 1080;

    MainWindow = Application.CreateWindow();
    if (MainWindow && MainWindow->Initialize(FProjectManager::GetProjectName(), WindowWidth, WindowHeight, 0, 0, Style))
    {
        MainWindow->Show(false);

        GToggleFullscreen.GetDelgate().AddRaw(MainWindow.Get(), &FGenericWindow::ToggleFullscreen);
    }
    else
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create Main Window");
        return false;
    }

    Application.RegisterMainViewport(MainWindow);

    TSharedPtr<ICursor> CursorDevice = Application.GetCursor();
    User = FUser::Make(0, CursorDevice);
    if (!User)
    {
        return false;
    }

    Application.RegisterUser(User);

    // Create standard textures
    uint8 Pixels[] = { 255, 255, 255, 255 };

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
    FRHISamplerStateInitializer SamplerCreateInfo;
    SamplerCreateInfo.AddressU       = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressV       = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressW       = ESamplerMode::Wrap;
    SamplerCreateInfo.ComparisonFunc = EComparisonFunc::Never;
    SamplerCreateInfo.Filter         = ESamplerFilter::Anistrotopic;
    SamplerCreateInfo.MaxAnisotropy  = 16;
    SamplerCreateInfo.MaxLOD         = FLT_MAX;
    SamplerCreateInfo.MinLOD         = -FLT_MAX;
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
    BaseMaterial->Initialize();

    /* Create the start scene */
    Scene = MakeShared<FScene>();

    /* Create windows */
    TSharedRef<FFrameProfilerWindow> ProfilerWindow = FFrameProfilerWindow::Make();
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
    delete this;
}
