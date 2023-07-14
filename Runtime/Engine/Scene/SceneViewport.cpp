#include "SceneViewport.h"

FSceneViewport::FSceneViewport(const TWeakPtr<FViewport>& InViewport)
    : IViewport()
    , Viewport(InViewport)
    , Scene(nullptr)
{
}

FSceneViewport::~FSceneViewport()
{
    Viewport = nullptr;
    Scene    = nullptr;
}

FResponse FSceneViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnAnalogGamepadEvent(AnalogGamepadEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just make a keyevent for now
        const FKeyEvent KeyEvent(CursorEvent.GetKey(), CursorEvent.GetModifierKeys(), false, CursorEvent.IsDown());
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just make a keyevent for now
        const FKeyEvent KeyEvent(CursorEvent.GetKey(), CursorEvent.GetModifierKeys(), false, CursorEvent.IsDown());
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        // NOTE: Just send to the first playercontroller for now
        PlayerController->GetPlayerInput()->OnCursorEvent(CursorEvent);
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
