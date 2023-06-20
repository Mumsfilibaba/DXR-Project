#include "Viewport.h"

FViewport::FViewport()
    : RHIViewport(nullptr)
    , Window(nullptr)
    , ViewportInterface(nullptr)
{
}

FViewport::~FViewport()
{
    CHECK(RHIViewport == nullptr);
}

bool FViewport::InitializeRHI(const FViewportInitializer& Initializer)
{
    Window = MakeSharedRef<FGenericWindow>(Initializer.Window);
    if (!Window)
    {
        DEBUG_BREAK();
        return false;
    }

    TSharedRef<FRHIViewport> NewViewport = RHICreateViewport(FRHIViewportDesc()
        .SetWindowHandle(Initializer.Window->GetPlatformHandle())
        .SetWidth(Initializer.Width)
        .SetHeight(Initializer.Height)
        .SetColorFormat(EFormat::R8G8B8A8_Unorm));

    if (!NewViewport)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        RHIViewport = NewViewport;
    }

    return true;
}

void FViewport::ReleaseRHI()
{
    RHIViewport.Reset();
}

FResponse FViewport::OnControllerButtonUp(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonUp(ControllerEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnControllerButtonDown(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonDown(ControllerEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnControllerButtonAnalog(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonAnalog(ControllerEvent) : FResponse::Unhandled();
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

FResponse FViewport::OnMouseMove(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseButtonDown(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseButtonUp(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseEntered(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseLeft(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseDoubleClick(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnWindowResized(const FWindowEvent& WindowEvent)
{
    return FResponse::Unhandled();
}

FResponse FViewport::OnWindowClosed(const FWindowEvent& WindowEvent)
{
    return FResponse::Unhandled();
}