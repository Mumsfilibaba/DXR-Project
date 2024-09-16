#include "Window.h"
#include "CoreApplication/Generic/GenericWindow.h"

FWindow::FWindow()
    : FWidget()
    , Title()
    , Overlay()
    , Content()
    , PlatformWindow(nullptr)
    , ParentWindow()
    , ChildWindows()
    , ScreenPosition()
    , ScreenSize()
    , WindowMode(EWindowMode::Windowed)
{
}

FWindow::~FWindow()
{
}

void FWindow::Initialize(const FInitializer& Initializer)
{
    Title          = Initializer.Title;
    ParentWindow   = Initializer.ParentWindow;
    ScreenPosition = Initializer.Position;
    ScreenSize     = Initializer.Size;
    WindowMode     = Initializer.WindowMode;
}

void FWindow::SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed)
{
    OnWindowClosed = InOnWindowClosed;
}

void FWindow::SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved)
{
    OnWindowMoved = InOnWindowMoved;
}

void FWindow::NotifyWindowDestroyed()
{
    OnWindowClosed.ExecuteIfBound();
}

void FWindow::NotifyWindowActivationChanged(bool bIsActive)
{
}

void FWindow::SetScreenPosition(const FIntVector2& NewPosition)
{
    ScreenPosition = NewPosition;
    OnWindowMoved.ExecuteIfBound(ScreenPosition);
}

void FWindow::SetScreenSize(const FIntVector2& NewSize)
{
    ScreenSize = NewSize;
}

void FWindow::SetTitle(const FString& InTitle)
{
    Title = InTitle;

    if (PlatformWindow)
    {
        PlatformWindow->SetTitle(Title);
    }
}

void FWindow::SetWindowMode(EWindowMode InWindowMode)
{
    if (PlatformWindow)
    {
        EWindowStyleFlags StyleFlags = EWindowStyleFlags::None;
        if (InWindowMode == EWindowMode::Windowed)
        {
            StyleFlags = EWindowStyleFlags::Default;
        }
        else
        {
            DEBUG_BREAK();
        }

        PlatformWindow->SetStyle(StyleFlags);
    }

    WindowMode = InWindowMode;
}

float FWindow::GetWindowDpiScale() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->GetWindowDpiScale();
    }

    return 1.0f;
}

uint32 FWindow::GetWidth() const
{
    return static_cast<uint32>(ScreenSize.x);
}

uint32 FWindow::GetHeight() const
{
    return static_cast<uint32>(ScreenSize.y);
}

TSharedPtr<FWidget> FWindow::GetOverlay() const
{
    return Overlay;
}

TSharedPtr<FWidget> FWindow::GetContent() const
{
    return Content;
}

void FWindow::SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow)
{
    PlatformWindow = InPlatformWindow;
    CHECK(PlatformWindow != nullptr);
}

void FWindow::SetOverlay(const TSharedPtr<FWidget>& InOverlay)
{
    Overlay = InOverlay;
}

void FWindow::SetContent(const TSharedPtr<FWidget>& InContent)
{
    Content = InContent;
}

void FWindow::Minimize()
{
    if (PlatformWindow)
    {
        PlatformWindow->Minimize();
    }
}

void FWindow::Maximize()
{
    if (PlatformWindow)
    {
        PlatformWindow->Maximize();
    }
}

void FWindow::Restore()
{
    if (PlatformWindow)
    {
        PlatformWindow->Restore();
    }
}

bool FWindow::IsActive() const
{
    return FApplication::Get().GetFocusWindow().Get() == this;
}

FIntVector2 FWindow::GetPosition() const
{
    return ScreenPosition;
}

bool FWindow::IsMinimized() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->IsMinimized();
    }

    return false;
}

bool FWindow::IsMaximized() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->IsMaximized();
    }

    return false;
}

void FWindow::AddParentWindow(const TSharedPtr<FWindow>& InParentWindow)
{
    ParentWindow = InParentWindow;
}

bool FWindow::IsChildWindow(const TSharedPtr<FWindow>& InParentWindow) const
{
    TSharedRef<FGenericWindow> PlatformParentWindow;
    if (InParentWindow)
    {
        PlatformParentWindow = InParentWindow->GetPlatformWindow();
    }

    if (PlatformWindow)
    {
        return PlatformWindow->IsChildWindow(PlatformParentWindow);
    }

    return false;
}

void FWindow::SetWindowFocus()
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowFocus();
    }
}

void FWindow::SetWindowOpacity(float Alpha)
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowOpacity(Alpha);
    }
}
