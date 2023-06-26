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
#include "RHI/RHIInterface.h"
#include "RendererCore/TextureFactory.h"

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


void FEngine::CreateMainWindow()
{
    // TODO: This should be loaded from a config file
    TSharedRef<FGenericWindow> Window = FApplication::Get().CreateWindow(
        FGenericWindowInitializer()
        .SetTitle("Sandbox")
        .SetWidth(2560)
        .SetHeight(1440));

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
    
    if (!Viewport->InitializeRHI(
        FViewportInitializer()
        .SetWindow(MainWindow.Get())
        .SetWidth(2560)
        .SetHeight(1440)))
    {
        DEBUG_BREAK();
        return false;
    }

    // Set the main-viewport
    MainViewport = Viewport;

    // Now we show the window
    MainWindow->Show();

    FApplication::Get().RegisterMainViewport(MainViewport);
    return true;
}

struct FDemoWidget final : public FWidget
{
    virtual void Paint() override final
    {
        static bool ShowDemoWindow = true;

        if (ShowDemoWindow)
        {
            ImGui::ShowDemoWindow();
        }

        static ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        static bool ShowAnotherWindow = false;
        
        {
            static float SliderValue = 0.0f;
            static int32 Counter     = 0;

            ImGui::Begin("Hello, world!");

            ImGui::Text("This is some useful text.");
            ImGui::Checkbox("Demo Window", &ShowDemoWindow);
            ImGui::Checkbox("Another Window", &ShowAnotherWindow);

            ImGui::SliderFloat("float", &SliderValue, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&ClearColor);

            if (ImGui::Button("Button"))
            {
                Counter++;
            }

            ImGui::SameLine();
            ImGui::Text("counter = %d", Counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        if (ShowAnotherWindow)
        {
            ImGui::Begin("Another Window", &ShowAnotherWindow);
            ImGui::Text("Hello from another window!");

            if (ImGui::Button("Close Me"))
            {
                ShowAnotherWindow = false;
            }

            ImGui::End();
        }
    }
};

bool FEngine::Init()
{
    CreateMainWindow();

    if (!CreateMainViewport())
    {
        return false;
    }

    // Demo Window
    FWidgetRef DemoWindow = MakeShared<FDemoWidget>();
    FApplication::Get().AddWidget(DemoWindow);

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

    // Create a new scene
    Scene = new FScene();

    // Create a SceneViewport
    SceneViewport = MakeShared<FSceneViewport>(MainViewport);
    SceneViewport->SetScene(Scene);
    MainViewport->SetViewportInterface(SceneViewport);

    /* Create Widgets */
    ProfilerWidget = MakeShared<FFrameProfilerWidget>();
    FApplication::Get().AddWidget(ProfilerWidget);

    ConsoleWidget = MakeShared<FConsoleWidget>();
    FApplication::Get().AddWidget(ConsoleWidget);
    return true;
}

bool FEngine::Start()
{
    if (Scene)
    {
        Scene->Start();
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

    if (Scene)
    {
        Scene->Tick(DeltaTime);
    }
}

void FEngine::Release()
{
    if (Scene)
    {
        SceneViewport->SetScene(nullptr);
        delete Scene;
    }

    FAssetManager::Release();

    FMeshImporter::Release();

    MainViewport->ReleaseRHI();
}

void FEngine::Exit()
{
    RequestEngineExit("Normal Exit");
}