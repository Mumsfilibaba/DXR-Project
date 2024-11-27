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
    ViewportInfo.Width        = static_cast<uint16>(WindowSize.X);
    ViewportInfo.Height       = static_cast<uint16>(WindowSize.Y);

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

FEventResponse FSceneViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnAxisEvent(AnalogGamepadEvent.GetAnalogSource(), AnalogGamepadEvent.GetAnalogValue());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
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
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
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
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first player-controller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
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
    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnFocusLost()
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just reset the states
        PlayerController->GetPlayerInput()->ClearInputStates();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FSceneViewport::OnFocusGained() 
{
    return FEventResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
