#include "Window.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "Application/Application.h"

FWindow::FWindow()
    : FWidget()
    , Title()
    , Overlay()
    , Content()
    , PlatformWindow(nullptr)
    , ScreenPosition()
    , ScreenSize()
    , StyleFlags(EWindowStyleFlags::None)
{
}

FWindow::~FWindow()
{
}

void FWindow::Initialize(const FInitializer& Initializer)
{
    Title          = Initializer.Title;
    ScreenPosition = Initializer.Position;
    ScreenSize     = Initializer.Size;
    StyleFlags     = Initializer.StyleFlags;
}

void FWindow::Tick()
{
    FRectangle NewBounds;
    NewBounds.Position = ScreenPosition;
    NewBounds.Width    = ScreenSize.x;
    NewBounds.Height   = ScreenSize.y;
    SetScreenRectangle(NewBounds);

    if (Content)
    {
        Content->Tick();
    }

    if (Overlay)
    {
        Overlay->Tick();
    }
}

bool FWindow::IsWindow() const
{
    return true;
}

void FWindow::FindChildrenUnderCursor(const FIntVector2& ScreenCursorPosition, FWidgetPath& OutParentWidgets)
{
    FRectangle InScreenRectangle = GetScreenRectangle();
    if (InScreenRectangle.EncapsualtesPoint(ScreenCursorPosition))
    {
        const EVisibility CurrentVisibility = GetVisibility();
        if (OutParentWidgets.AcceptVisbility(CurrentVisibility))
        {
            OutParentWidgets.Add(CurrentVisibility, AsSharedPtr());

            if (Content)
            {
                Content->FindChildrenUnderCursor(ScreenCursorPosition, OutParentWidgets);
            }

            if (Overlay)
            {
                Overlay->FindChildrenUnderCursor(ScreenCursorPosition, OutParentWidgets);
            }
        }
    }
}

void FWindow::SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed)
{
    OnWindowClosed = InOnWindowClosed;
}

void FWindow::SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved)
{
    OnWindowMoved = InOnWindowMoved;
}

void FWindow::SetOnWindowResized(const FOnWindowResized& InOnWindowResized)
{
    OnWindowResized = InOnWindowResized;
}

void FWindow::NotifyWindowDestroyed()
{
    if (PlatformWindow)
    {
        PlatformWindow->Destroy();
    }

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
    if (ScreenSize != NewSize)
    {
        ScreenSize = NewSize;
        OnWindowResized.ExecuteIfBound(NewSize);
    }
}

FIntVector2 FWindow::GetScreenPosition() const
{
    return ScreenPosition;
}

FIntVector2 FWindow::GetScreenSize() const
{
    return ScreenSize;
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
    return FApplicationInterface::Get().GetFocusWindow().Get() == this;
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

float FWindow::GetWindowDpiScale() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->GetWindowDpiScale();
    }

    return 1.0f;
}

void FWindow::SetTitle(const FString& InTitle)
{
    Title = InTitle;

    if (PlatformWindow)
    {
        PlatformWindow->SetTitle(Title);
    }
}

void FWindow::SetStyle(EWindowStyleFlags InStyleFlags)
{
    if (PlatformWindow)
    {
        if (StyleFlags != InStyleFlags)
        {
            PlatformWindow->SetStyle(InStyleFlags);
            StyleFlags = InStyleFlags;
        }
    }
}

void FWindow::SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow)
{
    PlatformWindow = InPlatformWindow;
    
    if (PlatformWindow)
    {
        FWindowShape WindowShape;
        PlatformWindow->GetWindowShape(WindowShape);
        
        ScreenSize     = FIntVector2(WindowShape.Width, WindowShape.Height);
        ScreenPosition = WindowShape.Position;
    }
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
