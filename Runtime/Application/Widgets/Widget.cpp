#include "Widget.h"

FWidget::FWidget()
    : TSharedFromThis<FWidget>()
    , ParentWidget()
    , Visibility(EVisibility::Visible)
    , Bounds()
{
}

FWidget::~FWidget()
{
}

FResponse FWidget::OnAnalogGamepadChange(const FAnalogGamepadEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnKeyDown(const FKeyEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnKeyUp(const FKeyEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnKeyChar(const FKeyEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseMove(const FCursorEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseButtonDown(const FCursorEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseButtonUp(const FCursorEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseScroll(const FCursorEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseDoubleClick(const FCursorEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseLeft(const FCursorEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnMouseEntered(const FCursorEvent&)
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnFocusLost()
{
    return FResponse::Unhandled();
}

FResponse FWidget::OnFocusGained()
{
    return FResponse::Unhandled();
}
