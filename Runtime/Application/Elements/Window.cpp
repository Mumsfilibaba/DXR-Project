#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Window.h"

TSharedPtr<FWindow> FWindow::Create(const FDesc& Desc)
{
    TSharedPtr<FWindow> NewInstance = MakeSharedPtr<FWindow>();
    if (NewInstance)
    {
        NewInstance->Initialize(Desc);
    }

    return NewInstance;
}

FWindow::FWindow()
    : FVisualElement()
    , Title()
    , OnWindowClosedDelegate()
    , OnWindowMovedDelegate()
    , OnWindowResizedDelegate()
    , OnWindowFocusChangedDelegate()
    , CachedPosition()
    , CachedSize()
    , StyleFlags(EWindowStyleFlags::None)
    , bActivateOnShow(true)
    , bAcceptsInput(true)
    , bShowOnCreate(true)
    , Overlay()
    , Content()
    , PlatformWindow(nullptr)
{
}

FWindow::~FWindow()
{
}

void FWindow::Initialize(const FDesc& Desc)
{
    Title               = Desc.Title;
    CachedPosition      = Desc.Position;
    CachedSize          = Desc.Size;
    StyleFlags          = Desc.StyleFlags;
    ParentWindow = Desc.ParentWindow;
    bActivateOnShow     = Desc.bActivateOnShow;
    bAcceptsInput       = Desc.bAcceptsInput;
    bShowOnCreate       = Desc.bShowOnCreate;

    // Windows should always receive focus, if the OS puts focus on the platform-window
    FVisualElement::SetActivationPolicy(EElementActivationPolicy::AutoFocusOnWindowActivate);
}

void FWindow::Tick(const FRectangle& AssignedBounds)
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

bool FWindow::IsWindow() const
{
    return true;
}

bool FWindow::SupportsKeyboardFocus() const
{
    return true;
}

int32 FWindow::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    int32 MaxLayerId = LayerId;

    if (Content && Content->IsVisible())
    {
        const FDrawGeometry ContentGeometry(Content->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = Content->OnDraw(ContentGeometry, OutCommandList, LayerId + 1);
    }

    if (Overlay && Overlay->IsVisible())
    {
        const FDrawGeometry OverlayGeometry(Overlay->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = Overlay->OnDraw(OverlayGeometry, OutCommandList, MaxLayerId + 1);
    }

    return MaxLayerId;
}

void FWindow::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (Content)
    {
        OutChildren.Add(Content);
    }

    if (Overlay)
    {
        OutChildren.Add(Overlay);
    }
}

void FWindow::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutParentElements)
{
    FRectangle WindowBounds = GetContentRectangle();
    if (WindowBounds.EncapsulatesPoint(ClientPosition))
    {
        const EVisibility CurrentVisibility = GetVisibility();
        if (OutParentElements.AcceptVisbility(CurrentVisibility))
        {
            OutParentElements.Add(CurrentVisibility, AsSharedPtr());

            const bool bIsOverlayModal = Overlay && Overlay->IsVisible() && Overlay->CapturesAllInput();

            if (Content && !bIsOverlayModal)
            {
                Content->FindChildrenContainingPoint(ClientPosition, OutParentElements);
            }

            if (Overlay)
            {
                Overlay->FindChildrenContainingPoint(ClientPosition, OutParentElements);
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

void FWindow::SetOnWindowFocusChanged(const FOnWindowFocusChanged& InOnWindowFocusChanged)
{
    OnWindowFocusChangedDelegate = InOnWindowFocusChanged;
}

void FWindow::OnWindowDestroyed()
{
    if (PlatformWindow)
    {
        PlatformWindow->Destroy();
    }

    OnWindowClosedDelegate.ExecuteIfBound();
}

void FWindow::OnWindowFocusChanged(bool)
{
    OnWindowFocusChangedDelegate.ExecuteIfBound();
}

void FWindow::OnWindowMoved(const IntVector2& InPosition)
{
    if (CachedPosition != InPosition)
    {
        SetPosition(InPosition);

        OnWindowMovedDelegate.ExecuteIfBound(InPosition);
    }
}

void FWindow::OnWindowResize(const IntVector2& InSize)
{
    if (CachedSize != InSize)
    {
        SetSize(InSize);

        OnWindowResizedDelegate.ExecuteIfBound(InSize);
    }
}

void FWindow::MoveTo(const IntVector2& InPosition)
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

void FWindow::Resize(const IntVector2& InSize)
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

void FWindow::SetSize(const IntVector2& InSize)
{
    CachedSize = InSize;
}

void FWindow::SetPosition(const IntVector2& InPosition)
{
    CachedPosition = InPosition;
}

IntVector2 FWindow::GetSize() const
{
    return CachedSize;
}

IntVector2 FWindow::GetPosition() const
{
    return CachedPosition;
}

uint32 FWindow::GetWidth() const
{
    return static_cast<uint32>(CachedSize.X);
}

uint32 FWindow::GetHeight() const
{
    return static_cast<uint32>(CachedSize.Y);
}

TSharedPtr<FVisualElement> FWindow::GetOverlay() const
{
    return Overlay;
}

TSharedPtr<FVisualElement> FWindow::GetContent() const
{
    return Content;
}

void FWindow::SetOverlay(const TSharedPtr<FVisualElement>& InOverlay)
{
    Overlay = InOverlay;
    if (Overlay)
    {
        Overlay->SetParentElement(AsWeakPtr());
    }
}

void FWindow::SetContent(const TSharedPtr<FVisualElement>& InContent)
{
    Content = InContent;
    if (Content)
    {
        Content->SetParentElement(AsWeakPtr());
    }
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
    return FApplication::Get().GetFocusWindow().Get() == this;
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

FWindowTitleBarMetrics FWindow::GetTitleBarMetrics() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->GetTitleBarMetrics();
    }

    return FWindowTitleBarMetrics();
}

void FWindow::SetTitleBarRegions(const FWindowTitleBarRegions& InRegions)
{
    if (PlatformWindow)
    {
        PlatformWindow->SetTitleBarRegions(InRegions);
    }
}

void FWindow::SetTitle(const String& InTitle)
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

void FWindow::SetAcceptsInput(bool bInAcceptsInput)
{
    if (bAcceptsInput != bInAcceptsInput)
    {
        bAcceptsInput = bInAcceptsInput;

        if (PlatformWindow)
        {
            PlatformWindow->SetAcceptsInput(bInAcceptsInput);
        }
    }
}

void FWindow::SetPlatformWindow(const TSharedRef<IPlatformWindow>& InPlatformWindow)
{
    PlatformWindow = InPlatformWindow;
    
    if (PlatformWindow)
    {
        FWindowShape WindowShape;
        PlatformWindow->GetWindowShape(WindowShape);

        CachedSize     = IntVector2(WindowShape.Width, WindowShape.Height);
        CachedPosition = WindowShape.Position;
        
        PlatformWindow->GetTitle(Title);
        
        StyleFlags    = PlatformWindow->GetStyle();
        bAcceptsInput = PlatformWindow->GetAcceptsInput();
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
