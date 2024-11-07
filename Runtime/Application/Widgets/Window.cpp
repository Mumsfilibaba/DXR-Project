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
    , CachedPosition()
    , CachedSize()
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
    CachedPosition = Initializer.Position;
    CachedSize     = Initializer.Size;
    StyleFlags     = Initializer.StyleFlags;
}

void FWindow::Tick()
{
    FRectangle NewContentRectangle;
    NewContentRectangle.Position = CachedPosition;
    NewContentRectangle.Width    = CachedSize.x;
    NewContentRectangle.Height   = CachedSize.y;
    SetContentRectangle(NewContentRectangle);

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

void FWindow::FindChildrenContainingPoint(const FIntVector2& Point, FWidgetPath& OutParentWidgets)
{
    FRectangle WindowBounds = GetContentRectangle();
    if (WindowBounds.EncapsualtesPoint(Point))
    {
        const EVisibility CurrentVisibility = GetVisibility();
        if (OutParentWidgets.AcceptVisbility(CurrentVisibility))
        {
            OutParentWidgets.Add(CurrentVisibility, AsSharedPtr());

            if (Content)
            {
                Content->FindChildrenContainingPoint(Point, OutParentWidgets);
            }

            if (Overlay)
            {
                Overlay->FindChildrenContainingPoint(Point, OutParentWidgets);
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
    if (CachedPosition != InPosition)
    {
        // Set the cached position
        SetPosition(InPosition);

        // Notify that this window was resized
        OnWindowMovedDelegate.ExecuteIfBound(InPosition);
    }
}

void FWindow::OnWindowResize(const FIntVector2& InSize)
{
    if (CachedSize != InSize)
    {
        // Set the cached size
        SetSize(InSize);

        // Notify that this window got resized
        OnWindowResizedDelegate.ExecuteIfBound(InSize);
    }
}

void FWindow::MoveTo(const FIntVector2& InPosition)
{
    if (CachedPosition != InPosition)
    {
        // Set the cached position
        SetPosition(InPosition);

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
    if (CachedSize != InSize)
    {
        // Set the cached size
        SetSize(InSize);

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

void FWindow::SetSize(const FIntVector2& InSize)
{
    CachedSize = InSize;
}

void FWindow::SetPosition(const FIntVector2& InPosition)
{
    CachedPosition = InPosition;
}

FIntVector2 FWindow::GetSize() const
{
    return CachedSize;
}

FIntVector2 FWindow::GetPosition() const
{
    return CachedPosition;
}

uint32 FWindow::GetWidth() const
{
    return static_cast<uint32>(CachedSize.x);
}

uint32 FWindow::GetHeight() const
{
    return static_cast<uint32>(CachedSize.y);
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

void FWindow::Show(bool bFocus)
{
    if (PlatformWindow)
    {
        PlatformWindow->Show(bFocus);
    }
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

float FWindow::GetWindowDPIScale() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->GetWindowDPIScale();
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
        // Cache the size and position
        FWindowShape WindowShape;
        PlatformWindow->GetWindowShape(WindowShape);

        CachedSize     = FIntVector2(WindowShape.Width, WindowShape.Height);
        CachedPosition = WindowShape.Position;
        
        // Cache the title
        PlatformWindow->GetTitle(Title);
        
        // Cache the style
        StyleFlags = PlatformWindow->GetStyle();
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
