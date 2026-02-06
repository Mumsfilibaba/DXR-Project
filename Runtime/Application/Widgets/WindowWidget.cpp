#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "Application/Application.h"
#include "Application/Widgets/WindowWidget.h"

FWindowWidget::FWindowWidget()
    : FWidget()
    , Title()
    , OnWindowClosedDelegate()
    , OnWindowMovedDelegate()
    , OnWindowResizedDelegate()
    , OnWindowFocusChangedDelegate()
    , CachedPosition()
    , CachedSize()
    , Overlay()
    , Content()
    , PlatformWindow(nullptr)
    , StyleFlags(EWindowStyleFlags::None)
    , bActivateOnShow(true)
{
}

FWindowWidget::~FWindowWidget()
{
}

void FWindowWidget::Initialize(const FInitializer& Initializer)
{
    Title              = Initializer.Title;
    CachedPosition     = Initializer.Position;
    CachedSize         = Initializer.Size;
    StyleFlags         = Initializer.StyleFlags;
    ParentWindowWidget = Initializer.ParentWindow;
    bActivateOnShow    = Initializer.bActivateOnShow;

    // Windows should always receive focus, if the OS puts focus on the platform-window
    FWidget::SetActivationPolicy(EWidgetActivationPolicy::AutoFocusOnWindowActivate);
}

void FWindowWidget::Tick(const FRectangle& AssignedBounds)
{
    SetContentRectangle(AssignedBounds);

    if (Content)
    {
        Content->Tick(AssignedBounds);
    }

    if (Overlay)
    {
        Overlay->Tick(AssignedBounds);
    }
}

bool FWindowWidget::IsWindow() const
{
    return true;
}

void FWindowWidget::FindChildrenContainingPoint(const FIntVector2& Point, FWidgetPath& OutParentWidgets)
{
    FRectangle WindowBounds = GetContentRectangle();
    if (WindowBounds.EncapsulatesPoint(Point))
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

void FWindowWidget::SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed)
{
    OnWindowClosedDelegate = InOnWindowClosed;
}

void FWindowWidget::SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved)
{
    OnWindowMovedDelegate = InOnWindowMoved;
}

void FWindowWidget::SetOnWindowResized(const FOnWindowResized& InOnWindowResized)
{
    OnWindowResizedDelegate = InOnWindowResized;
}

void FWindowWidget::SetOnWindowFocusChanged(const FOnWindowFocusChanged& InOnWindowFocusChanged)
{
    OnWindowFocusChangedDelegate = InOnWindowFocusChanged;
}

void FWindowWidget::OnWindowDestroyed()
{
    if (PlatformWindow)
    {
        PlatformWindow->Destroy();
    }

    OnWindowClosedDelegate.ExecuteIfBound();
}

void FWindowWidget::OnWindowFocusChanged(bool)
{
    OnWindowFocusChangedDelegate.ExecuteIfBound();
}

void FWindowWidget::OnWindowMoved(const FIntVector2& InPosition)
{
    if (CachedPosition != InPosition)
    {
        SetPosition(InPosition);

        OnWindowMovedDelegate.ExecuteIfBound(InPosition);
    }
}

void FWindowWidget::OnWindowResize(const FIntVector2& InSize)
{
    if (CachedSize != InSize)
    {
        SetSize(InSize);

        OnWindowResizedDelegate.ExecuteIfBound(InSize);
    }
}

void FWindowWidget::MoveTo(const FIntVector2& InPosition)
{
    if (CachedPosition != InPosition)
    {
        SetPosition(InPosition);

        OnWindowMovedDelegate.ExecuteIfBound(InPosition);

        if (PlatformWindow)
        {
            PlatformWindow->SetWindowPos(InPosition.X, InPosition.Y);
        }
    }
}

void FWindowWidget::Resize(const FIntVector2& InSize)
{
    if (CachedSize != InSize)
    {
        SetSize(InSize);

        OnWindowResizedDelegate.ExecuteIfBound(InSize);

        if (PlatformWindow)
        {
            FWindowShape WindowShape(InSize.X, InSize.Y);
            PlatformWindow->SetWindowShape(WindowShape, false);
        }
    }
}

void FWindowWidget::SetSize(const FIntVector2& InSize)
{
    CachedSize = InSize;
}

void FWindowWidget::SetPosition(const FIntVector2& InPosition)
{
    CachedPosition = InPosition;
}

FIntVector2 FWindowWidget::GetSize() const
{
    return CachedSize;
}

FIntVector2 FWindowWidget::GetPosition() const
{
    return CachedPosition;
}

uint32 FWindowWidget::GetWidth() const
{
    return static_cast<uint32>(CachedSize.X);
}

uint32 FWindowWidget::GetHeight() const
{
    return static_cast<uint32>(CachedSize.Y);
}

TSharedPtr<FWidget> FWindowWidget::GetOverlay() const
{
    return Overlay;
}

TSharedPtr<FWidget> FWindowWidget::GetContent() const
{
    return Content;
}

void FWindowWidget::SetOverlay(const TSharedPtr<FWidget>& InOverlay)
{
    Overlay = InOverlay;
}

void FWindowWidget::SetContent(const TSharedPtr<FWidget>& InContent)
{
    Content = InContent;
}

void FWindowWidget::Show(bool bFocus)
{
    if (PlatformWindow)
    {
        PlatformWindow->Show(bFocus);
    }
}

void FWindowWidget::Minimize()
{
    if (PlatformWindow)
    {
        PlatformWindow->Minimize();
    }
}

void FWindowWidget::Maximize()
{
    if (PlatformWindow)
    {
        PlatformWindow->Maximize();
    }
}

void FWindowWidget::Restore()
{
    if (PlatformWindow)
    {
        PlatformWindow->Restore();
    }
}

bool FWindowWidget::IsActive() const
{
    return FApplication::Get().GetFocusWindow().Get() == this;
}

bool FWindowWidget::IsMinimized() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->IsMinimized();
    }

    return false;
}

bool FWindowWidget::IsMaximized() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->IsMaximized();
    }

    return false;
}

float FWindowWidget::GetWindowDPIScale() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->GetWindowDPIScale();
    }

    return 1.0f;
}

void FWindowWidget::SetTitle(const FString& InTitle)
{
    Title = InTitle;

    if (PlatformWindow)
    {
        PlatformWindow->SetTitle(Title);
    }
}

void FWindowWidget::SetStyle(EWindowStyleFlags InStyleFlags)
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

void FWindowWidget::SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow)
{
    PlatformWindow = InPlatformWindow;
    
    if (PlatformWindow)
    {
        FWindowShape WindowShape;
        PlatformWindow->GetWindowShape(WindowShape);

        CachedSize     = FIntVector2(WindowShape.Width, WindowShape.Height);
        CachedPosition = WindowShape.Position;
        
        PlatformWindow->GetTitle(Title);
        
        StyleFlags = PlatformWindow->GetStyle();
    }
}

void FWindowWidget::SetFocus()
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowFocus();
    }
}

void FWindowWidget::SetOpacity(float Alpha)
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowOpacity(Alpha);
    }
}
