#include "SceneViewport.h"

FSceneViewport::FSceneViewport(const TWeakPtr<FViewport>& InViewport)
    : IViewport()
    , Viewport(InViewport)
    , World(nullptr)
{
}

FSceneViewport::~FSceneViewport()
{
    Viewport = nullptr;
    World    = nullptr;
}

FResponse FSceneViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnAxisEvent(AnalogGamepadEvent.GetAnalogSource(), AnalogGamepadEvent.GetAnalogValue());
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown(), KeyEvent.IsRepeat());
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), CursorEvent.IsDown(), false);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        // PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(CursorEvent.GetKey(), true, false);
        return FResponse::Handled();
    }

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
