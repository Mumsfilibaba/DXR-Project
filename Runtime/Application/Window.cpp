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

bool FWindow::Create()
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

    if (Width == 0 || Height == 0 || WindowMode == EWindowMode::None)
    {
        LOG_ERROR("[FWindow]: Width, Height or WindowMode is not Initialized");
        return false;
    }

    // TODO: Control these somehow
    FWindowStyle WindowStyle
    { 
        EWindowStyleFlag::Titled |
        EWindowStyleFlag::Closable |
        EWindowStyleFlag::Resizeable |
        EWindowStyleFlag::Maximizable |
        EWindowStyleFlag::Minimizable 
    };

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

bool FWindow::IsActiveWindow() const 
{
    if (NativeWindow)
    {
        return NativeWindow->IsActiveWindow();
    }

    return false; 
}

bool FWindow::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (Viewport)
    {
        return Viewport->OnKeyDown(KeyEvent);
    }

    return false;
}
    
bool FWindow::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (Viewport)
    {
        return Viewport->OnKeyUp(KeyEvent);
    }

    return false;
}
    
bool FWindow::OnKeyChar(FKeyCharEvent KeyCharEvent)
{
    if (Viewport)
    {
        return Viewport->OnKeyChar(KeyCharEvent);
    }

    return false;
}

bool FWindow::OnMouseMove(const FMouseMovedEvent& MouseEvent)
{
    if (Viewport)
    {
        return Viewport->OnMouseMove(MouseEvent);
    }

    return false;
}
    
bool FWindow::OnMouseDown(const FMouseButtonEvent& MouseEvent)
{
    if (Viewport)
    {
        return Viewport->OnMouseDown(MouseEvent);
    }

    return false;
}
    
bool FWindow::OnMouseUp(const FMouseButtonEvent& MouseEvent)
{
    if (Viewport)
    {
        return Viewport->OnMouseUp(MouseEvent);
    }

    return false;
}
    
bool FWindow::OnMouseScroll(const FMouseScrolledEvent& MouseEvent)
{
    if (Viewport)
    {
        return Viewport->OnMouseScroll(MouseEvent);
    }

    return false;
}
    
bool FWindow::OnMouseEntered()
{
    if (Viewport)
    {
        return Viewport->OnMouseEntered();
    }

    return false;
}
    
bool FWindow::OnMouseLeft()
{
    if (Viewport)
    {
        return Viewport->OnMouseLeft();
    }

    return false;
}

bool FWindow::OnWindowResized(const FWindowResizedEvent& ResizeEvent)
{
    if (Viewport)
    {
        return Viewport->OnWindowResized(ResizeEvent);
    }

    Width  = ResizeEvent.Width;
    Height = ResizeEvent.Height;
    return false;
}
    
bool FWindow::OnWindowMoved(const FWindowMovedEvent& InMoveEvent)
{
    if (Viewport)
    {
        return Viewport->OnWindowMove(InMoveEvent);
    }

    return false;
}
    
bool FWindow::OnWindowFocusGained()
{
    if (Viewport)
    {
        return Viewport->OnWindowFocusGained();
    }

    return false;
}
    
bool FWindow::OnWindowFocusLost()
{
    if (Viewport)
    {
       return  Viewport->OnWindowFocusLost();
    }

    return false;
}
    
bool FWindow::OnWindowClosed()
{
    if (Viewport)
    {
        return Viewport->OnWindowClosed();
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

void FWindow::SetTitle(const FString& InTitle)
{
    if (NativeWindow)
    {
        NativeWindow->SetTitle(Title);
    }

    Title = InTitle;
}

void FWindow::SetWidth(uint32 InWidth)
{
    CHECK(InWidth != 0);
    Width = InWidth;

    if (NativeWindow)
    {
        FWindowShape NewWindowShape{ Width, Height };
        NativeWindow->SetWindowShape(NewWindowShape, false);
    }
}

void FWindow::SetHeight(uint32 InHeight)
{
    CHECK(InHeight != 0);
    Height = InHeight;

    if (NativeWindow)
    {
        FWindowShape NewWindowShape{ Width, Height };
        NativeWindow->SetWindowShape(NewWindowShape, false);
    }
}