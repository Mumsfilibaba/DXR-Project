#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/WindowElement.h"

TSharedPtr<FWindowElement> FWindowElement::Create(const FInitializer& Initializer)
{
    TSharedPtr<FWindowElement> NewElement = MakeSharedPtr<FWindowElement>();
    if (NewElement)
    {
        NewElement->Initialize(Initializer);
    }

    return NewElement;
}

FWindowElement::FWindowElement()
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
    , Overlay()
    , Content()
    , PlatformWindow(nullptr)
{
}

FWindowElement::~FWindowElement()
{
}

void FWindowElement::Initialize(const FInitializer& Initializer)
{
    Title               = Initializer.Title;
    CachedPosition      = Initializer.Position;
    CachedSize          = Initializer.Size;
    StyleFlags          = Initializer.StyleFlags;
    ParentWindowElement = Initializer.ParentWindow;
    bActivateOnShow     = Initializer.bActivateOnShow;
    bAcceptsInput       = Initializer.bAcceptsInput;

    // Windows should always receive focus, if the OS puts focus on the platform-window
    FVisualElement::SetActivationPolicy(EElementActivationPolicy::AutoFocusOnWindowActivate);
}

void FWindowElement::Tick(const FRectangle& AssignedBounds)
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

bool FWindowElement::IsWindow() const
{
    return true;
}

bool FWindowElement::SupportsKeyboardFocus() const
{
    return true;
}

int32 FWindowElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
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

void FWindowElement::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
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

void FWindowElement::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutParentElements)
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

void FWindowElement::SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed)
{
    OnWindowClosedDelegate = InOnWindowClosed;
}

void FWindowElement::SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved)
{
    OnWindowMovedDelegate = InOnWindowMoved;
}

void FWindowElement::SetOnWindowResized(const FOnWindowResized& InOnWindowResized)
{
    OnWindowResizedDelegate = InOnWindowResized;
}

void FWindowElement::SetOnWindowFocusChanged(const FOnWindowFocusChanged& InOnWindowFocusChanged)
{
    OnWindowFocusChangedDelegate = InOnWindowFocusChanged;
}

void FWindowElement::OnWindowDestroyed()
{
    if (PlatformWindow)
    {
        PlatformWindow->Destroy();
    }

    OnWindowClosedDelegate.ExecuteIfBound();
}

void FWindowElement::OnWindowFocusChanged(bool)
{
    OnWindowFocusChangedDelegate.ExecuteIfBound();
}

void FWindowElement::OnWindowMoved(const IntVector2& InPosition)
{
    if (CachedPosition != InPosition)
    {
        SetPosition(InPosition);

        OnWindowMovedDelegate.ExecuteIfBound(InPosition);
    }
}

void FWindowElement::OnWindowResize(const IntVector2& InSize)
{
    if (CachedSize != InSize)
    {
        SetSize(InSize);

        OnWindowResizedDelegate.ExecuteIfBound(InSize);
    }
}

void FWindowElement::MoveTo(const IntVector2& InPosition)
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

void FWindowElement::Resize(const IntVector2& InSize)
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

void FWindowElement::SetSize(const IntVector2& InSize)
{
    CachedSize = InSize;
}

void FWindowElement::SetPosition(const IntVector2& InPosition)
{
    CachedPosition = InPosition;
}

IntVector2 FWindowElement::GetSize() const
{
    return CachedSize;
}

IntVector2 FWindowElement::GetPosition() const
{
    return CachedPosition;
}

uint32 FWindowElement::GetWidth() const
{
    return static_cast<uint32>(CachedSize.X);
}

uint32 FWindowElement::GetHeight() const
{
    return static_cast<uint32>(CachedSize.Y);
}

TSharedPtr<FVisualElement> FWindowElement::GetOverlay() const
{
    return Overlay;
}

TSharedPtr<FVisualElement> FWindowElement::GetContent() const
{
    return Content;
}

void FWindowElement::SetOverlay(const TSharedPtr<FVisualElement>& InOverlay)
{
    Overlay = InOverlay;
    if (Overlay)
    {
        Overlay->SetParentElement(AsWeakPtr());
    }
}

void FWindowElement::SetContent(const TSharedPtr<FVisualElement>& InContent)
{
    Content = InContent;
    if (Content)
    {
        Content->SetParentElement(AsWeakPtr());
    }
}

void FWindowElement::Show(bool bFocus)
{
    if (PlatformWindow)
    {
        PlatformWindow->Show(bFocus);
    }
}

void FWindowElement::Minimize()
{
    if (PlatformWindow)
    {
        PlatformWindow->Minimize();
    }
}

void FWindowElement::Maximize()
{
    if (PlatformWindow)
    {
        PlatformWindow->Maximize();
    }
}

void FWindowElement::Restore()
{
    if (PlatformWindow)
    {
        PlatformWindow->Restore();
    }
}

bool FWindowElement::IsActive() const
{
    return FApplication::Get().GetFocusWindow().Get() == this;
}

bool FWindowElement::IsMinimized() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->IsMinimized();
    }

    return false;
}

bool FWindowElement::IsMaximized() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->IsMaximized();
    }

    return false;
}

float FWindowElement::GetWindowDPIScale() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->GetWindowDPIScale();
    }

    return 1.0f;
}

FWindowTitleBarMetrics FWindowElement::GetTitleBarMetrics() const
{
    if (PlatformWindow)
    {
        return PlatformWindow->GetTitleBarMetrics();
    }

    return FWindowTitleBarMetrics();
}

void FWindowElement::SetTitleBarRegions(const FWindowTitleBarRegions& InRegions)
{
    if (PlatformWindow)
    {
        PlatformWindow->SetTitleBarRegions(InRegions);
    }
}

void FWindowElement::SetTitle(const String& InTitle)
{
    Title = InTitle;

    if (PlatformWindow)
    {
        PlatformWindow->SetTitle(Title);
    }
}

void FWindowElement::SetStyle(EWindowStyleFlags InStyleFlags)
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

void FWindowElement::SetAcceptsInput(bool bInAcceptsInput)
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

void FWindowElement::SetPlatformWindow(const TSharedRef<IPlatformWindow>& InPlatformWindow)
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

void FWindowElement::SetFocus()
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowFocus();
    }
}

void FWindowElement::SetOpacity(float Alpha)
{
    if (PlatformWindow)
    {
        PlatformWindow->SetWindowOpacity(Alpha);
    }
}
