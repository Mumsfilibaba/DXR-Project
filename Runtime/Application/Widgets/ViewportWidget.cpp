#include "ViewportWidget.h"
#include "Core/Misc/OutputDeviceLogger.h"

FViewportWidget::FViewportWidget()
    : FWidget()
    , ViewportInterface(nullptr) 
{
}

FViewportWidget::~FViewportWidget()
{
}

void FViewportWidget::Initialize(const FInitializer& Initializer)
{
    ViewportInterface = Initializer.ViewportInterface;
}

void FViewportWidget::Tick(const FRectangle& AssignedBounds)
{
    SetContentRectangle(AssignedBounds);
}

FEventResponse FViewportWidget::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    return ViewportInterface ? ViewportInterface->OnAnalogGamepadChange(AnalogGamepadEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyDown(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyUp(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyChar(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseMove(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseScroll(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnHighPrecisionMouseInput(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnFocusLost()
{
    return ViewportInterface ? ViewportInterface->OnFocusLost() : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnFocusGained()
{
    return ViewportInterface ? ViewportInterface->OnFocusGained() : FEventResponse::Unhandled();
}
