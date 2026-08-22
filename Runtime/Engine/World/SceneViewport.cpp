#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"
#include "Application/Elements/ViewportElement.h"
#include "Engine/World/Actors/PlayerInput.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/SceneViewport.h"
#include "RHI/RHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FSceneViewport::FSceneViewport(const TWeakPtr<FViewportElement>& InViewport)
    : IViewport()
    , World(nullptr)
    , Viewport(InViewport)
    , RHISwapChain(nullptr)
    , HighPrecisionMouseDelta()
    , MouseRestorePosition()
    , bPlayerInputEnabled(true)
    , bMouseCaptured(false)
    , bCursorWasVisible(true)
    , bDiscardCaptureWarpDelta(false)
{
}

FSceneViewport::~FSceneViewport()
{
    CHECK(RHISwapChain == nullptr);

    // Going away mid-run would otherwise leave the cursor hidden and confined
    ReleaseMouse();

    Viewport = nullptr;
    World    = nullptr;
}

bool FSceneViewport::InitializeRHI()
{
    TSharedPtr<FViewportElement> ViewportElement;
    if (Viewport.IsExpired())
    {
        LOG_INFO("No valid viewport");
        return false;
    }
    else
    {
        ViewportElement = Viewport.ToSharedPtr();
    }

    TSharedPtr<FWindowElement> WindowElement = FApplication::Get().FindWindow(ViewportElement);
    if (!WindowElement)
    {
        return false;
    }

    const IntVector2 WindowSize = WindowElement->GetSize();
    FRHISwapChainDesc SwapChainDesc;
    SwapChainDesc.Width        = static_cast<uint16>(WindowSize.X);
    SwapChainDesc.Height       = static_cast<uint16>(WindowSize.Y);
    SwapChainDesc.WindowHandle = WindowElement->GetPlatformWindow()->GetPlatformHandle();
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

    if (bMouseCaptured)
    {
        FApplication::Get().ConfineCursorToRect(GetCaptureWindow(), GetCaptureRect());
    }
}

bool FSceneViewport::CaptureMouse()
{
    const TSharedPtr<FWindowElement>   Window          = GetCaptureWindow();
    const TSharedPtr<FViewportElement> ViewportElement = GetViewportElement();

    if (bMouseCaptured || !ViewportElement || !Window)
    {
        return false;
    }

    FApplication& Application = FApplication::Get();

    bCursorWasVisible    = Application.IsCursorVisible();
    MouseRestorePosition = Application.GetCursorPosition();

    if (!Application.SetHighPrecisionMouseMode(Window, EHighPrecisionMouseMode::Enabled))
    {
        return false;
    }

    const FRectangle CaptureRect = GetCaptureRect();

    Application.ShowCursor(false);
    Application.ConfineCursorToRect(Window, CaptureRect);
    Application.SetCursorPosition(IntVector2(CaptureRect.Position.X + (CaptureRect.Width / 2), CaptureRect.Position.Y + (CaptureRect.Height / 2)));

    if (!Application.CaptureMouse(ViewportElement))
    {
        Application.ReleaseCursorConfinement();
        Application.SetHighPrecisionMouseMode(Window, EHighPrecisionMouseMode::Disabled);
        Application.SetCursorPosition(MouseRestorePosition);
        Application.ShowCursor(bCursorWasVisible);
        return false;
    }

    bMouseCaptured           = true;
    bDiscardCaptureWarpDelta = true;

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->ClearMouseDelta();
    }

    return true;
}

void FSceneViewport::ReleaseMouse()
{
    if (!bMouseCaptured)
    {
        return;
    }

    bMouseCaptured           = false;
    bDiscardCaptureWarpDelta = false;

    if (FApplication::IsInitialized())
    {
        FApplication& Application = FApplication::Get();
        Application.ReleaseMouseCapture(GetViewportElement());
        Application.ReleaseCursorConfinement();

        Application.SetHighPrecisionMouseMode(GetCaptureWindow(), EHighPrecisionMouseMode::Disabled);
        Application.SetCursorPosition(MouseRestorePosition);
        Application.ShowCursor(bCursorWasVisible);
    }
}

TSharedPtr<FWindowElement> FSceneViewport::GetCaptureWindow() const
{
    if (!FApplication::IsInitialized() || !Viewport.IsValid())
    {
        return nullptr;
    }

    return FApplication::Get().FindWindow(TSharedPtr<FViewportElement>(Viewport));
}

FRectangle FSceneViewport::GetCaptureRect() const
{
    const TSharedPtr<FWindowElement> Window = GetCaptureWindow();

    if (Viewport.IsValid())
    {
        const FRectangle& ViewportArea = Viewport->GetContentRectangle();
        if (ViewportArea.Width > 0 && ViewportArea.Height > 0)
        {
            FRectangle ScreenRect = ViewportArea;
            if (Window)
            {
                ScreenRect.Position += Window->GetPosition();
            }

            return ScreenRect;
        }
    }

    FRectangle WindowRect;
    if (Window)
    {
        const IntVector2 WindowSize = Window->GetSize();
        WindowRect.Position = Window->GetPosition();
        WindowRect.Width    = WindowSize.X;
        WindowRect.Height   = WindowSize.Y;
    }

    return WindowRect;
}

void FSceneViewport::SetPlayerInputEnabled(bool bEnabled)
{
    if (bPlayerInputEnabled == bEnabled)
    {
        return;
    }

    bPlayerInputEnabled     = bEnabled;
    HighPrecisionMouseDelta = IntVector2();

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        if (bPlayerInputEnabled)
        {
            PlayerController->GetPlayerInput()->ClearMouseDelta();
        }
        else
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

    if (bMouseCaptured && KeyEvent.GetKey() == Keys::Escape)
    {
        ReleaseMouse();
        return FEventResponse::Handled();
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

    if (!bMouseCaptured)
    {
        CaptureMouse();
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
    const IntVector2 Delta = CursorEvent.GetHighPrecisionDelta();

    if (bPlayerInputEnabled && bDiscardCaptureWarpDelta)
    {
        constexpr int32 CaptureWarpDeltaThreshold = 64;
        if (Math::Abs(Delta.X) > CaptureWarpDeltaThreshold || Math::Abs(Delta.Y) > CaptureWarpDeltaThreshold)
        {
            bDiscardCaptureWarpDelta = false;
            return FEventResponse::Handled();
        }

        bDiscardCaptureWarpDelta = false;
    }

    if (!bPlayerInputEnabled)
    {
        HighPrecisionMouseDelta.X += Delta.X;
        HighPrecisionMouseDelta.Y += Delta.Y;
        
        return FEventResponse::Handled();
    }

    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnHighPrecisionMouseInput(Delta);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnFocusLost()
{
    ReleaseMouse();

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
