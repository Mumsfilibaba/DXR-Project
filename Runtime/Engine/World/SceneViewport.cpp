#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"
#include "Application/Widgets/ViewportWidget.h"
#include "Engine/World/Actors/PlayerInput.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/SceneViewport.h"
#include "RHI/RHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FSceneViewport::FSceneViewport(const TWeakPtr<FViewportWidget>& InViewport)
    : IViewport()
    , World(nullptr)
    , Viewport(InViewport)
    , RHISwapChain(nullptr)
    , bPlayerInputEnabled(true)
    , HighPrecisionMouseDelta()
{
}

FSceneViewport::~FSceneViewport()
{
    CHECK(RHISwapChain == nullptr);

    Viewport = nullptr;
    World    = nullptr;
}

bool FSceneViewport::InitializeRHI()
{
    TSharedPtr<FViewportWidget> ViewportWidget;
    if (Viewport.IsExpired())
    {
        LOG_INFO("No valid viewport");
        return false;
    }
    else
    {
        ViewportWidget = Viewport.ToSharedPtr();
    }

    TSharedPtr<FWindowWidget> WindowWidget = FApplication::Get().FindWindowWidget(ViewportWidget);
    if (!WindowWidget)
    {
        return false;
    }

    const IntVector2 WindowSize = WindowWidget->GetSize();
    FRHISwapChainDesc SwapChainDesc;
    SwapChainDesc.Width        = static_cast<uint16>(WindowSize.X);
    SwapChainDesc.Height       = static_cast<uint16>(WindowSize.Y);
    SwapChainDesc.WindowHandle = WindowWidget->GetPlatformWindow()->GetPlatformHandle();
    SwapChainDesc.ColorFormat  = EFormat::Unknown;
    SwapChainDesc.ColorSpace   = EColorSpace::Unknown;
    SwapChainDesc.Usage        = ESwapChainUsageFlags::RenderTarget;
    SwapChainDesc.bFramePacing = true;

    FRHISwapChainRef NewSwapChain = RHI::CreateSwapChain(SwapChainDesc);
    if (!NewSwapChain)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        RHISwapChain = NewSwapChain;
    }

    return true;
}

void FSceneViewport::ReleaseRHI()
{
    CHECK(RHISwapChain->GetRefCount() == 1);
    RHISwapChain.Reset();
}

void FSceneViewport::Tick()
{
    if (World)
    {
        FCameraComponent* Camera = World->GetActiveCamera();
        if (Camera && Viewport.IsValid())
        {
            const FRectangle& ViewportArea = Viewport->GetContentRectangle();
            Camera->UpdateProjectionMatrix(static_cast<float>(ViewportArea.Width), static_cast<float>(ViewportArea.Height));
        }
    }
}

void FSceneViewport::SetPlayerInputEnabled(bool bEnabled)
{
    if (bPlayerInputEnabled == bEnabled)
    {
        return;
    }

    bPlayerInputEnabled     = bEnabled;
    HighPrecisionMouseDelta = IntVector2();

    if (!bPlayerInputEnabled)
    {
        if (FPlayerController* PlayerController = GetFirstPlayerController())
        {
            PlayerController->GetPlayerInput()->ClearInputStates();
        }
    }
}

IntVector2 FSceneViewport::ConsumeHighPrecisionMouseDelta()
{
    const IntVector2 Delta = HighPrecisionMouseDelta;
    HighPrecisionMouseDelta = IntVector2();
    return Delta;
}

FEventResponse FSceneViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnAxisEvent(AnalogGamepadEvent.GetAnalogSource(), AnalogGamepadEvent.GetAnalogValue());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyChar(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    if (!bPlayerInputEnabled)
    {
        return FEventResponse::Unhandled();
    }

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), true, false);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    if (!bPlayerInputEnabled)
    {
        const IntVector2 Delta = CursorEvent.GetCursorPos();
        HighPrecisionMouseDelta.X += Delta.X;
        HighPrecisionMouseDelta.Y += Delta.Y;
        
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnFocusLost()
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->ClearInputStates();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnFocusGained() 
{
    return FEventResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
