#include "Engine.h"

#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Debug/Profiler.h"

#include "Rendering/Resources/TextureFactory.h"

#include "CoreRHI/RHICore.h"

/* Console vars */
CConsoleCommand GToggleFullscreen;
CConsoleCommand GExit;

/* Global engine instance */
TSharedPtr<CEngine> GEngine;

CEngine::CEngine()
{
}

CEngine::~CEngine()
{
}

bool CEngine::Init()
{
    /* Register for events about exiting the application */
    OnApplicationExitHandle = CApplication::Get().ApplicationExitEvent.AddRaw( this, &CEngine::OnApplicationExit );

    /* Get notified when the main-window closes */
    WindowHandler.WindowClosedDelegate.BindLambda( [this]( const SWindowClosedEvent& ClosedEvent )
    {
        if ( this->MainWindow == ClosedEvent.Window )
        {
            this->Exit();
        }
    } );

    CApplication::Get().AddWindowMessageHandler( &WindowHandler );

    const uint32 Style =
        WindowStyleFlag_Titled |
        WindowStyleFlag_Closable |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    MainWindow = CApplication::Get().MakeWindow();
    if ( MainWindow && MainWindow->Init( "DXR Engine", 1920, 1080, Style ) )
    {
        MainWindow->Show( false );

        GToggleFullscreen.OnExecute.AddRaw( MainWindow.Get(), &CCoreWindow::ToggleFullscreen );
        INIT_CONSOLE_COMMAND( "a.ToggleFullscreen", &GToggleFullscreen );
    }
    else
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "Failed to create Engine" );
        return false;
    }

    ICursor* CursorDevice = CApplication::Get().GetCursor();
    User = CApplicationUser::Make( 0, CursorDevice );
    if ( !User )
    {
        return false;
    }

    CApplication::Get().RegisterUser( User );

    GExit.OnExecute.AddRaw( this, &CEngine::Exit );
    INIT_CONSOLE_COMMAND( "a.Exit", &GExit );

    // Create standard textures
    uint8 Pixels[] =
    {
        255,
        255,
        255,
        255
    };

    BaseTexture = CTextureFactory::LoadFromMemory( Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm );
    if ( !BaseTexture )
    {
        LOG_WARNING( "Failed to create BaseTexture" );
    }
    else
    {
        BaseTexture->SetName( "BaseTexture" );
    }

    Pixels[0] = 127;
    Pixels[1] = 127;
    Pixels[2] = 255;

    BaseNormal = CTextureFactory::LoadFromMemory( Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm );
    if ( !BaseNormal )
    {
        LOG_WARNING( "Failed to create BaseNormal-Texture" );
    }
    else
    {
        BaseNormal->SetName( "BaseNormal" );
    }

    /* Create material sampler (Used for now by all materials) */
    SSamplerStateCreateInfo SamplerCreateInfo;
    SamplerCreateInfo.AddressU = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressV = ESamplerMode::Wrap;
    SamplerCreateInfo.AddressW = ESamplerMode::Wrap;
    SamplerCreateInfo.ComparisonFunc = EComparisonFunc::Never;
    SamplerCreateInfo.Filter = ESamplerFilter::Anistrotopic;
    SamplerCreateInfo.MaxAnisotropy = 16;
    SamplerCreateInfo.MaxLOD = FLT_MAX;
    SamplerCreateInfo.MinLOD = -FLT_MAX;
    SamplerCreateInfo.MipLODBias = 0.0f;

    BaseMaterialSampler = RHICreateSamplerState( SamplerCreateInfo );

    /* Base material */
    SMaterialDesc MaterialDesc;
    MaterialDesc.AO = 1.0f;
    MaterialDesc.Metallic = 0.0f;
    MaterialDesc.Roughness = 1.0f;
    MaterialDesc.EnableHeight = 0;
    MaterialDesc.Albedo = CVector3( 1.0f );

    BaseMaterial = MakeShared<CMaterial>( MaterialDesc );
    BaseMaterial->AlbedoMap = GEngine->BaseTexture;
    BaseMaterial->NormalMap = GEngine->BaseNormal;
    BaseMaterial->RoughnessMap = GEngine->BaseTexture;
    BaseMaterial->AOMap = GEngine->BaseTexture;
    BaseMaterial->MetallicMap = GEngine->BaseTexture;
    BaseMaterial->Init();

    /* Create the start scene */
    Scene = MakeShared<CScene>();

    return true;
}

bool CEngine::Start()
{
    IsRunning = true;
    return true;
}

void CEngine::Tick( CTimestamp DeltaTime )
{
    TRACE_FUNCTION_SCOPE();

    if ( Scene )
    {
        Scene->Tick( DeltaTime );
    }
}

bool CEngine::Release()
{
    /* Unregister for events */
    CApplication::Get().ApplicationExitEvent.Unbind( OnApplicationExitHandle );

    return true;
}

void CEngine::OnApplicationExit( int32 )
{
    IsRunning = false;
}

void CEngine::Exit()
{
    PlatformApplicationMisc::RequestExit( 0 );
}
