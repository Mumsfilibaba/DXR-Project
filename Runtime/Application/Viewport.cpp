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
    
    FRHIViewportDesc ViewportDesc;
    ViewportDesc.WindowHandle = Initializer.Window->GetPlatformHandle();
    ViewportDesc.ColorFormat  = EFormat::R8G8B8A8_Unorm;
    ViewportDesc.Width        = static_cast<uint16>(Initializer.Width);
    ViewportDesc.Height       = static_cast<uint16>(Initializer.Height);
    
    FRHIViewportRef NewViewport = RHICreateViewport(ViewportDesc);
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

FResponse FViewport::OnMouseMove(const FCursorEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseButtonDown(const FCursorEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseButtonUp(const FCursorEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseScroll(const FCursorEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseScroll(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnMouseDoubleClick(const FCursorEvent& MouseEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(MouseEvent) : FResponse::Unhandled();
}

FResponse FViewport::OnWindowResized(const FWindowEvent& WindowEvent)
{
    return FResponse::Unhandled();
}

FResponse FViewport::OnWindowFocusLost(const FWindowEvent& WindowEvent)
{
    if (WindowEvent.GetWindow() == Window)
    {
        return ViewportInterface ? ViewportInterface->OnFocusLost() : FResponse::Unhandled();
    }

    return FResponse::Unhandled();
}

FResponse FViewport::OnWindowFocusGained(const FWindowEvent& WindowEvent)
{
    if (WindowEvent.GetWindow() == Window)
    {
        return ViewportInterface ? ViewportInterface->OnFocusGained() : FResponse::Unhandled();
    }

    return FResponse::Unhandled();
}

FResponse FViewport::OnMouseLeft(const FWindowEvent& WindowEvent)
{
    if (WindowEvent.GetWindow() == Window)
    {
        return ViewportInterface ? ViewportInterface->OnMouseLeft() : FResponse::Unhandled();
    }

    return FResponse::Unhandled();
}

FResponse FViewport::OnMouseEntered(const FWindowEvent& WindowEvent)
{
    if (WindowEvent.GetWindow() == Window)
    {
        return ViewportInterface ? ViewportInterface->OnMouseEntered() : FResponse::Unhandled();
    }

    return FResponse::Unhandled();
}

FResponse FViewport::OnWindowClosed(const FWindowEvent& WindowEvent)
{
    return FResponse::Unhandled();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
