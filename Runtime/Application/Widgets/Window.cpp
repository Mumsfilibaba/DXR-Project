#include "Window.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "Application/Application.h"

FWindow::FWindow()
    : FWidget()
    , Title()
    , OnWindowClosedDelegate()
    , OnWindowMovedDelegate()
    , OnWindowResizedDelegate()
    , OnWindowActivationChangedDelegate()
    , ScreenPosition()
    , ScreenSize()
    , Overlay()
    , Content()
    , PlatformWindow(nullptr)
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
    OnWindowClosedDelegate = InOnWindowClosed;
}

void FWindow::SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved)
{
    OnWindowMovedDelegate = InOnWindowMoved;
}

void FWindow::SetOnWindowResized(const FOnWindowResized& InOnWindowResized)
{
    OnWindowResizedDelegate = InOnWindowResized;
}

void FWindow::OnWindowDestroyed()
{
    if (PlatformWindow)
    {
        PlatformWindow->Destroy();
    }

    OnWindowClosedDelegate.ExecuteIfBound();
}

void FWindow::OnWindowActivationChanged(bool)
{
    OnWindowActivationChangedDelegate.ExecuteIfBound();
}

void FWindow::OnWindowMoved(const FIntVector2& InPosition)
{
    if (ScreenPosition != InPosition)
    {
        // Set the cached position
        SetCachedPosition(InPosition);
        
        // Notify that this window was resized
        OnWindowMovedDelegate.ExecuteIfBound(InPosition);
    }
}

void FWindow::OnWindowResize(const FIntVector2& InSize)
{
    if (ScreenSize != InSize)
    {
        // Set the cached size
        SetCachedSize(InSize);
        
        // Notify that this window got resized
        OnWindowResizedDelegate.ExecuteIfBound(InSize);
    }
}

void FWindow::MoveTo(const FIntVector2& InPosition)
{
    if (ScreenPosition != InPosition)
    {
        // Set the cached position
        SetCachedPosition(InPosition);
        
        // Notify that this window was resized
        OnWindowMovedDelegate.ExecuteIfBound(InPosition);
        
        // Set the actual size of the platform window
        if (PlatformWindow)
        {
            PlatformWindow->SetWindowPos(InPosition.x, InPosition.y);
        }
    }
}

void FWindow::Resize(const FIntVector2& InSize)
{
    if (ScreenSize != InSize)
    {
        // Set the cached size
        SetCachedSize(InSize);
        
        // Notify that this window got resized
        OnWindowResizedDelegate.ExecuteIfBound(InSize);
        
        // Set the actual size of the platform window
        if (PlatformWindow)
        {
            FWindowShape WindowShape(InSize.x, InSize.y);
            PlatformWindow->SetWindowShape(WindowShape, false);
        }
    }
}

void FWindow::SetCachedSize(const FIntVector2& InSize)
{
    ScreenSize = InSize;
}

void FWindow::SetCachedPosition(const FIntVector2& InPosition)
{
    ScreenPosition = InPosition;
}

FIntVector2 FWindow::GetCachedPosition() const
{
    return ScreenPosition;
}

FIntVector2 FWindow::GetCachedSize() const
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

void FWindow::SetFocus()
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowFocus();
    }
}

void FWindow::SetOpacity(float Alpha)
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowOpacity(Alpha);
    }
}
