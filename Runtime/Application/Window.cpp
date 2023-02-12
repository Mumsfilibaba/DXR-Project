#include "Window.h"
#include "Application.h"
#include "Core/Misc/OutputDeviceLogger.h"

FWindow::FWindow()
    : Viewport(nullptr)
    , NativeWindow(nullptr)
    , Title()
    , WindowMode(EWindowMode::None)
    , Width{0}
    , Height{0}
    , bIsVisible{false}
    , bIsMaximized{false}
{ }

bool FWindow::Initialize(const FWindowInitializer& Initializer)
{
    TSharedPtr<FGenericApplication> PlatfromApplication = FApplication::Get().GetPlatformApplication();
    if (!PlatfromApplication)
    {
        LOG_ERROR("[FWindow]: PlatformApplication is not initialized");
        return false;
    }

    NativeWindow = PlatfromApplication->CreateWindow();
    if (!NativeWindow)
    {
        LOG_ERROR("[FWindow]: Failed to create NativeWindow");
        return false;
    }

    if (Initializer.Width == 0 || Initializer.Height == 0 || Initializer.WindowMode == EWindowMode::None)
    {
        LOG_ERROR("[FWindow]: Width, Height or WindowMode is not Initialized");
        return false;
    }

    // TODO: Control these somehow
    const FWindowStyle WindowStyle
    { 
        EWindowStyleFlag::Titled |
        EWindowStyleFlag::Closable |
        EWindowStyleFlag::Resizeable |
        EWindowStyleFlag::Maximizable |
        EWindowStyleFlag::Minimizable 
    };

    WindowMode = Initializer.WindowMode;
    Width  = Initializer.Width;
    Height = Initializer.Height;

    return NativeWindow->Initialize(Title, Width, Height, 0, 0, WindowStyle);
}

void FWindow::Show(bool bMaximized)
{
    if (NativeWindow)
    {
        NativeWindow->Show(bMaximized);
    }

    bIsVisible = true;
}

void FWindow::Close()
{
    if (NativeWindow)
    {
        NativeWindow->Close();
    }

    bIsVisible = false;
}

void FWindow::Minimize()
{
    if (NativeWindow)
    {
        NativeWindow->Minimize();
    }
}

void FWindow::Maximize()
{
    if (NativeWindow)
    {
        NativeWindow->Maximize();
    }
}

void FWindow::Restore()
{
    if (NativeWindow)
    {
        NativeWindow->Restore();
    }
}

bool FWindow::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return Content ? Content->OnKeyDown(KeyEvent) : false;
}
    
bool FWindow::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return Content ? Content->OnKeyUp(KeyEvent) : false;
}
    
bool FWindow::OnKeyChar(FKeyCharEvent KeyCharEvent)
{
    return Content ? Content->OnKeyChar(KeyCharEvent) : false;
}

bool FWindow::OnMouseMove(const FMouseMovedEvent& MouseEvent)
{
    return Content ? Content->OnMouseMove(MouseEvent) : false;
}
    
bool FWindow::OnMouseDown(const FMouseButtonEvent& MouseEvent)
{
    return Content ? Content->OnMouseDown(MouseEvent) : false;
}
    
bool FWindow::OnMouseUp(const FMouseButtonEvent& MouseEvent)
{
    return Content ? Content->OnMouseUp(MouseEvent) : false;
}
    
bool FWindow::OnMouseScroll(const FMouseScrolledEvent& MouseEvent)
{
    return Content ? Content->OnMouseScroll(MouseEvent) : false;
}
    
bool FWindow::OnMouseEntered()
{
    return Content ? Content->OnMouseEntered() : false;
}
    
bool FWindow::OnMouseLeft()
{
    return Content ? Content->OnMouseLeft() : false;
}

bool FWindow::OnWindowResized(const FWindowResizedEvent& ResizeEvent)
{
    if (Content)
    {
        return Content->OnWindowResized(ResizeEvent);
    }

    Width  = ResizeEvent.Width;
    Height = ResizeEvent.Height;
    return false;
}
    
bool FWindow::OnWindowMoved(const FWindowMovedEvent& InMoveEvent)
{
    if (Content)
    {
        return Content->OnWindowMove(InMoveEvent);
    }

    return false;
}
    
bool FWindow::OnWindowFocusGained()
{
    return Content ? Content->OnWindowFocusGained() : false;
}
    
bool FWindow::OnWindowFocusLost()
{
    return Content ? Content->OnWindowFocusLost() : false;
}
    
bool FWindow::OnWindowClosed()
{
    if (Content)
    {
        return Content->OnWindowClosed();
    }

    return false;
}

void FWindow::SetWindowMode(EWindowMode InWindowMode)
{
    CHECK(InWindowMode != EWindowMode::Borderless && InWindowMode != EWindowMode::None);

    // TODO: Send this directly to the native window
    if (NativeWindow)
    {
        if (WindowMode != InWindowMode)
        {
            NativeWindow->ToggleFullscreen();
        }
    }

    WindowMode = InWindowMode;
}
