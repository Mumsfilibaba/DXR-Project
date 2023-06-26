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

FResponse FSceneViewport::OnControllerAnalog(const FControllerEvent& ControllerEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnControllerEvent(ControllerEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnControllerButtonDown(const FControllerEvent& ControllerEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnControllerEvent(ControllerEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnControllerButtonUp(const FControllerEvent& ControllerEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnControllerEvent(ControllerEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnKeyEvent(KeyEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseMove(const FMouseEvent& MouseEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnMouseEvent(MouseEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonDown(const FMouseEvent& MouseEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnMouseEvent(MouseEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonUp(const FMouseEvent& MouseEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnMouseEvent(MouseEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseScroll(const FMouseEvent& MouseEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnMouseEvent(MouseEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseDoubleClick(const FMouseEvent& MouseEvent)
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->OnMouseEvent(MouseEvent);
        return FResponse::Handled();
    }

    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnFocusLost()
{
    if (FPlayerController* PlayerController = GetFirstPlayerController())
    {
        PlayerController->GetPlayerInput()->ResetStates();
    }

    return FResponse::Unhandled();
}
