#include "Viewport.h"
#include "Window.h"
#include "Application.h"
#include "Project/ProjectManager.h"
#include "RHI/RHIInterface.h"

FViewport::FViewport(const TWeakPtr<FWindow>& InParentWindow)
    : FWidget(InParentWindow)
    , ViewportRHI(nullptr)
    , ViewportInterface(nullptr)
    , ParentWindow(InParentWindow)
{ }

bool FViewport::CreateRHI()
{
    if (!ParentWindow.IsValid())
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

void FViewport::DestroyRHI()
{
    ViewportRHI.Reset();
}

bool FViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnKeyDown(KeyEvent);
    }

    return false;
}

bool FViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnKeyUp(KeyEvent);
    }

    return false;
}

bool FViewport::OnKeyChar(FKeyCharEvent KeyCharEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnKeyChar(KeyCharEvent);
    }
    
    return false;
}

bool FViewport::OnMouseMove(const FMouseMovedEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseMove(MouseEvent);
    }

    return false;
}

bool FViewport::OnMouseDown(const FMouseButtonEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseDown(MouseEvent);
    }

    return false;
}

bool FViewport::OnMouseUp(const FMouseButtonEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseUp(MouseEvent);
    }

    return false;
}

bool FViewport::OnMouseScroll(const FMouseScrolledEvent& MouseEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseScroll(MouseEvent);
    }

    return false;
}

bool FViewport::OnMouseEntered()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseEntered();
    }

    return false;
}

bool FViewport::OnMouseLeft()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnMouseLeft();
    }

    return false;
}

bool FViewport::OnWindowResized(const FWindowResizedEvent& InResizeEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowResized(InResizeEvent);
    }

    return false;
}

bool FViewport::OnWindowMove(const FWindowMovedEvent& InMoveEvent)
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowMove(InMoveEvent);
    }

    return false;
}

bool FViewport::OnWindowFocusGained()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowFocusGained();
    }

    return false;
}

bool FViewport::OnWindowFocusLost()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowFocusLost();
    }

    return false;
}

bool FViewport::OnWindowClosed()
{
    if (ViewportInterface)
    {
        return ViewportInterface->OnWindowClosed();
    }

    return false;
}

FIntVector2 FViewport::GetSize() const
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