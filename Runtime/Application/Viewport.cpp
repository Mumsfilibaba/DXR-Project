#include "Viewport.h"
#include "Window.h"
#include "Application.h"
#include "Project/ProjectManager.h"
#include "RHI/RHIInterface.h"

FViewportElement::FViewportElement(const TWeakPtr<FElement>& InParentElement)
    : FElement(InParentElement)
    , ViewportRHI(nullptr)
    , ViewportInterface(nullptr)
{ }

bool FViewportElement::CreateRHI()
{
    TWeakPtr<FElement> ParentElement = GetParentElement();
    if (!ParentElement.IsValid())
    {
        DEBUG_BREAK();
        return false;
    }

    if (ViewportRHI)
    {
        DEBUG_BREAK();
        return true;
    }

    TSharedRef<FGenericWindow> NativeWindow = ParentWindow->GetNativeWindow();
    if (!NativeWindow)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIViewportDesc ViewportDesc(
        NativeWindow->GetPlatformHandle(),
        EFormat::R8G8B8A8_Unorm,
        EFormat::Unknown,
        static_cast<uint16>(NativeWindow->GetWidth()),
        static_cast<uint16>(NativeWindow->GetHeight()));

    ViewportRHI = RHICreateViewport(ViewportDesc);
    if (!ViewportRHI)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FViewportElement::DestroyRHI()
{
    ViewportRHI.Reset();
}

bool FViewportElement::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnKeyDown(KeyEvent);
    }

    return false;
}

bool FViewportElement::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnKeyUp(KeyEvent);
    }

    return false;
}

bool FViewportElement::OnKeyChar(FKeyCharEvent KeyCharEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnKeyChar(KeyCharEvent);
    }
    
    return false;
}

bool FViewportElement::OnMouseMove(const FMouseMovedEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseMove(MouseEvent);
    }

    return false;
}

bool FViewportElement::OnMouseDown(const FMouseButtonEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseDown(MouseEvent);
    }

    return false;
}

bool FViewportElement::OnMouseUp(const FMouseButtonEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseUp(MouseEvent);
    }

    return false;
}

bool FViewportElement::OnMouseScroll(const FMouseScrolledEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseScroll(MouseEvent);
    }

    return false;
}

bool FViewportElement::OnMouseEntered()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseEntered();
    }

    return false;
}

bool FViewportElement::OnMouseLeft()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseLeft();
    }

    return false;
}

bool FViewportElement::OnWindowResized(const FWindowResizedEvent& InResizeEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowResized(InResizeEvent);
    }

    return false;
}

bool FViewportElement::OnWindowMove(const FWindowMovedEvent& InMoveEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowMove(InMoveEvent);
    }

    return false;
}

bool FViewportElement::OnWindowFocusGained()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowFocusGained();
    }

    return false;
}

bool FViewportElement::OnWindowFocusLost()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowFocusLost();
    }

    return false;
}

bool FViewportElement::OnWindowClosed()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowClosed();
    }

    return false;
}

FIntVector2 FViewportElement::GetSize() const
{
    if (ParentWindow.IsValid())
    {
        return FIntVector2
        { 
            static_cast<int32>(ParentWindow->GetWidth()),
            static_cast<int32>(ParentWindow->GetHeight())
        };
    }

    return FIntVector2{ 0, 0 };
}