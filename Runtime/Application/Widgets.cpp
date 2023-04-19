#include "Widget.h"

FWidget::FWidget()
    : Visibility(Visibility_All)
    , Content(this)
    , LocalBounds()
    , DesktopBounds()
{
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FWidget::ArrangeChildren(FFilteredWidgets& OutArrangedChildren)
{
}

void FWidget::FindWidgetsUnderCursor(FIntVector2 ScreenPosition, FFilteredWidgets& OutChildren)
{
}

FResponse FWidget::OnControllerButtonUp(const FControllerEvent& ControllerEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnControllerButtonDown(const FControllerEvent& ControllerEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnControllerButtonAnalog(const FControllerEvent& ControllerEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseMove(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseButtonDown(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseButtonUp(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseEntered(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseLeft(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseDoubleClick(const FMouseEvent& MouseEvent)
{
    return FResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING