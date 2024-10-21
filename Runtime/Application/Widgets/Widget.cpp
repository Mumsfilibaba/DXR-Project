#include "Widget.h"
#include "Application/WidgetPath.h"

FWidget::FWidget()
    : TSharedFromThis<FWidget>()
    , Visibility(EVisibility::Visible)
    , ScreenRectangle()
    , ParentWidget()
{
}

FWidget::~FWidget()
{
}

void FWidget::Tick()
{
}

bool FWidget::IsWindow() const
{
    return false;
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

void FWidget::FindParentWidgets(FWidgetPath& OutRootPath)
{
    if (ParentWidget.IsValid())
    {
        ParentWidget->FindParentWidgets(OutRootPath);
    }

    OutRootPath.Add(Visibility, AsSharedPtr());
}

void FWidget::FindChildrenUnderCursor(const FIntVector2& ScreenCursorPosition, FWidgetPath& OutChildWidgets)
{
    if (ScreenRectangle.EncapsualtesPoint(ScreenCursorPosition))
    {
        OutChildWidgets.Add(Visibility, AsSharedPtr());
    }
}

void FWidget::SetVisibility(EVisibility InVisibility)
{
    Visibility = InVisibility;
}

void FWidget::SetParentWidget(const TWeakPtr<FWidget>& InParentWidget)
{
    ParentWidget = InParentWidget;
}

void FWidget::SetScreenRectangle(const FRectangle& InScreenRectangle)
{
    ScreenRectangle = InScreenRectangle;
}
