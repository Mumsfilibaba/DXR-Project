#include "Engine.h"

#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Modules/ModuleManager.h"

#include "Application/ApplicationInstance.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Engine/Resources/Material.h"
#include "Engine/Resources/TextureFactory.h"
#include "Engine/InterfaceWindows/GameConsoleWindow.h"
#include "Engine/InterfaceWindows/FrameProfilerWindow.h"
#include "Engine/Project/ProjectManager.h"

#include "RHI/RHIInstance.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

ENGINE_API CEngine* GEngine;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleCommands

CAutoConsoleCommand GExit("engine.Exit");
CAutoConsoleCommand GToggleFullscreen("viewport.ToggleFullscreen");

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Engine

CEngine* CEngine::Make()
{
    return dbg_new CEngine();
}

CEngine::CEngine()
{
	GExit.GetExecutedDelgate().AddRaw(this, &CEngine::Exit);
}

bool CEngine::Initialize()
{
    const uint32 Style =
        WindowStyleFlag_Titled |
        WindowStyleFlag_Closable |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    CApplicationInstance& Application = CApplicationInstance::Get();

    const uint32 WindowWidth  = 1920;
    const uint32 WindowHeight = 1080;

    MainWindow = Application.MakeWindow();
    if (MainWindow && MainWindow->Initialize(CProjectManager::GetProjectName(), WindowWidth, WindowHeight, 0, 0, Style))
    {
        MainWindow->Show(false);

        GToggleFullscreen.GetExecutedDelgate().AddRaw(MainWindow.Get(), &CPlatformWindow::ToggleFullscreen);
    }
    else
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to create Main Window");
        return false;
    }

    Application.RegisterMainViewport(MainWindow);

    TSharedPtr<ICursor> CursorDevice = Application.GetCursor();
    User = CApplicationUser::Make(0, CursorDevice);
    if (!User)
    {
        return false;
    }

    Application.RegisterUser(User);

    // Create standard textures
    uint8 Pixels[] = { 255, 255, 255, 255 };

    BaseTexture = CTextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
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

    BaseNormal = CTextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!BaseNormal)
    {
        LOG_WARNING("Failed to create BaseNormal-Texture");
    }
    else
    {
        BaseNormal->SetName("BaseNormal");
    }

    /* Create material sampler (Used for now by all materials) */
    SRHISamplerStateInfo SamplerCreateInfo;
    SamplerCreateInfo.AddressU = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressV = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressW = ESamplerMode::Wrap;
    SamplerCreateInfo.ComparisonFunc = EComparisonFunc::Never;
    SamplerCreateInfo.Filter = ESamplerFilter::Anistrotopic;
    SamplerCreateInfo.MaxAnisotropy = 16;
    SamplerCreateInfo.MaxLOD = FLT_MAX;
    SamplerCreateInfo.MinLOD = -FLT_MAX;
    SamplerCreateInfo.MipLODBias = 0.0f;

    BaseMaterialSampler = RHICreateSamplerState(SamplerCreateInfo);

    /* Base material */
    SMaterialDesc MaterialDesc;
    MaterialDesc.AO = 1.0f;
    MaterialDesc.Metallic = 0.0f;
    MaterialDesc.Roughness = 1.0f;
    MaterialDesc.EnableHeight = 0;
    MaterialDesc.Albedo = CVector3(1.0f);

    BaseMaterial = MakeShared<CMaterial>(MaterialDesc);
    BaseMaterial->AlbedoMap = GEngine->BaseTexture;
    BaseMaterial->NormalMap = GEngine->BaseNormal;
    BaseMaterial->RoughnessMap = GEngine->BaseTexture;
    BaseMaterial->AOMap = GEngine->BaseTexture;
    BaseMaterial->MetallicMap = GEngine->BaseTexture;
    BaseMaterial->Init();

    /* Create the start scene */
    Scene = MakeShared<CScene>();

    /* Create windows */
    TSharedRef<CFrameProfilerWindow> ProfilerWindow = CFrameProfilerWindow::Make();
    Application.AddWindow(ProfilerWindow);

    TSharedRef<CGameConsoleWindow> ConsoleWindow = CGameConsoleWindow::Make();
    Application.AddWindow(ConsoleWindow);

    return true;
}

bool CEngine::Start()
{
    Scene->Start();
    return true;
}

void CEngine::Tick(CTimestamp DeltaTime)
{
    TRACE_FUNCTION_SCOPE();

    if (Scene)
    {
        Scene->Tick(DeltaTime);
    }
}

bool CEngine::Release()
{
    return true;
}

void CEngine::Exit()
{
    PlatformApplicationMisc::RequestExit(0);
}

void CEngine::Destroy()
{
    delete this;
}
