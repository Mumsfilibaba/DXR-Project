#include "Viewport.h"

FViewport::FViewport()
    : FWidget()
    , ViewportInterface(nullptr) 
{
}

FViewport::~FViewport()
{
}

void FViewport::Initialize(const FInitializer& Initializer)
{
    ViewportInterface = Initializer.ViewportInterface;
}

FResponse FViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    return ViewportInterface ? ViewportInterface->OnAnalogGamepadChange(AnalogGamepadEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyDown(KeyEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyUp(KeyEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyChar(KeyEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseMove(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(CursorEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(CursorEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(CursorEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseScroll(CursorEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(CursorEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(CursorEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(CursorEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnFocusLost()
{
    return ViewportInterface ? ViewportInterface->OnFocusLost() : FResponse::Unhandled();
}

FResponse FViewport::OnFocusGained()
{
    return ViewportInterface ? ViewportInterface->OnFocusGained() : FResponse::Unhandled();
}
