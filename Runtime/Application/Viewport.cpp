#include "Viewport.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "RHI/RHIInterface.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

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

    FRHIViewportRef NewViewport = RHICreateViewport(
        FRHIViewportDesc()
        .SetWindowHandle(Initializer.Window->GetPlatformHandle())
        .SetWidth(static_cast<uint16>(Initializer.Width))
        .SetHeight(static_cast<uint16>(Initializer.Height))
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

FResponse FViewport::OnControllerAnalog(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerAnalog(ControllerEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnControllerButtonDown(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonDown(ControllerEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnControllerButtonUp(const FControllerEvent& ControllerEvent)
{
    return ViewportInterface ? ViewportInterface->OnControllerButtonUp(ControllerEvent) : FResponse::Unhandled();
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

FResponse FViewport::OnMouseScroll(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseScroll(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseDoubleClick(const FMouseEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnWindowResized(const FWindowEvent& WindowEvent)
{
    return FResponse::Unhandled();
}

FResponse FViewport::OnWindowFocusLost(const FWindowEvent& WindowEvent)
{
    return ViewportInterface ? ViewportInterface->OnFocusLost(WindowEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnWindowFocusGained(const FWindowEvent& WindowEvent)
{
    return ViewportInterface ? ViewportInterface->OnFocusGained(WindowEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseLeft(const FWindowEvent& WindowEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(WindowEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseEntered(const FWindowEvent& WindowEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(WindowEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnWindowClosed(const FWindowEvent& WindowEvent)
{
    return FResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING