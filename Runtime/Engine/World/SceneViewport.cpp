#include "SceneViewport.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Application.h"
#include "Application/Widgets/Viewport.h"
#include "Engine/World/Actors/PlayerInput.h"
#include "RHI/RHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FSceneViewport::FSceneViewport(const TWeakPtr<FViewport>& InViewport)
    : IViewport()
    , Viewport(InViewport)
    , RHIViewport(nullptr)
    , World(nullptr)
{
}

FSceneViewport::~FSceneViewport()
{
    CHECK(RHIViewport == nullptr);

    Viewport = nullptr;
    World    = nullptr;
}

bool FSceneViewport::InitializeRHI()
{
    TSharedPtr<FViewport> ViewportWidget;
    if (Viewport.IsExpired())
    {
        LOG_INFO("No valid viewport");
        return false;
    }
    else
    {
        ViewportWidget = Viewport.ToSharedPtr();
    }

    TSharedPtr<FWindow> WindowWidget = FApplicationInterface::Get().FindWindowWidget(ViewportWidget);
    if (!WindowWidget)
    {
        return false;
    }

    const FIntVector2 WindowSize = WindowWidget->GetSize();
    FRHIViewportInfo ViewportInfo;
    ViewportInfo.WindowHandle = WindowWidget->GetPlatformWindow()->GetPlatformHandle();
    ViewportInfo.ColorFormat  = EFormat::B8G8R8A8_Unorm; // TODO: We might want to use RGBA for all RHIs except Vulkan?
    ViewportInfo.Width        = static_cast<uint16>(WindowSize.x);
    ViewportInfo.Height       = static_cast<uint16>(WindowSize.y);

    FRHIViewportRef NewViewport = RHICreateViewport(ViewportInfo);
    if (!NewViewport)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        RHIViewport = NewViewport;
    }

    return true;
}

void FSceneViewport::ReleaseRHI()
{
    CHECK(RHIViewport->GetRefCount() == 1);
    RHIViewport.Reset();
}

FResponse FSceneViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnAxisEvent(AnalogGamepadEvent.GetAnalogSource(), AnalogGamepadEvent.GetAnalogValue());
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyChar(const FKeyEvent&)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), true, false);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnFocusLost()
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just reset the states
        PlayerController->GetPlayerInput()->ClearInputStates();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnFocusGained() 
{
    return FResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
