#include "Widget.h"
#include "Application/WidgetPath.h"

FWidget::FWidget()
    : TSharedFromThis<FWidget>()
    , Visibility(EVisibility::Visible)
    , ContentRectangle()
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

FEventResponse FWidget::OnAnalogGamepadChange(const FAnalogGamepadEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnKeyDown(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnKeyUp(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnKeyChar(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnMouseMove(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnMouseButtonDown(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnMouseButtonUp(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnMouseScroll(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnMouseDoubleClick(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnMouseLeft(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnMouseEntered(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnHighPrecisionMouseInput(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnFocusLost()
{
    return FEventResponse::Unhandled();
}

FEventResponse FWidget::OnFocusGained()
{
    return FEventResponse::Unhandled();
}

void FWidget::FindParentWidgets(FWidgetPath& OutRootPath)
{
    if (ParentWidget.IsValid())
    {
        ParentWidget->FindParentWidgets(OutRootPath);
    }

    OutRootPath.Add(Visibility, AsSharedPtr());
}

void FWidget::FindChildrenContainingPoint(const FIntVector2& ScreenCursorPosition, FWidgetPath& OutChildWidgets)
{
    if (ContentRectangle.EncapsulatesPoint(ScreenCursorPosition))
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

void FWidget::SetContentRectangle(const FRectangle& InContentRectangle)
{
    ContentRectangle = InContentRectangle;
}
