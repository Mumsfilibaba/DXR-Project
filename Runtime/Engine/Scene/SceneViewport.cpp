#include "SceneViewport.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FSceneViewport::FSceneViewport(const TWeakPtr<FViewportWidget>& InViewport)
    : IViewport()
    , Viewport(InViewport)
    , Scene(nullptr)
{
}

FSceneViewport::~FSceneViewport()
{
    Scene = nullptr;
}

FResponse FSceneViewport::OnControllerButtonUp(const FControllerEvent& ControllerEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnControllerButtonDown(const FControllerEvent& ControllerEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnControllerButtonAnalog(const FControllerEvent& ControllerEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseMove(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonDown(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseButtonUp(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseEntered(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseScroll(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseLeft(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FSceneViewport::OnMouseDoubleClick(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING