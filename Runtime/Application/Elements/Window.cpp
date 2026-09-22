#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/MenuHost.h"
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
    , CachedOpacity(1.0f)
    , StyleFlags(EWindowStyleFlags::None)
    , bActivateOnShow(true)
    , bAcceptsInput(true)
    , bShowOnCreate(true)
    , bHasExternalSurface(false)
    , bLayoutIsStale(false)
    , bCachedIsMaximized(false)
    , Overlay()
    , MenuHost()
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
    bHasExternalSurface = Desc.bHasExternalSurface;

    FVisualElement::SetActivationPolicy(EElementActivationPolicy::AutoFocusOnWindowActivate);
}

IntVector2 FWindow::PrepareDesiredSize()
{
    if (Content)
    {
        Content->PrepareDesiredSize();
    }

    return FVisualElement::PrepareDesiredSize();
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

    if (MenuHost)
    {
        MenuHost->Tick(AssignedBounds);
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
        QueueDeferredPainting(Overlay, FDrawGeometry(Overlay->GetContentRectangle(), AllottedGeometry.Scale));
    }

    if (MenuHost && MenuHost->IsVisible() && !MenuHost->IsEmpty())
    {
        QueueDeferredPainting(MenuHost, FDrawGeometry(MenuHost->GetContentRectangle(), AllottedGeometry.Scale));
    }

    return MaxLayerId;
}

void FWindow::QueueDeferredPainting(const TSharedPtr<FVisualElement>& Element, const FDrawGeometry& Geometry) const
{
    if (!Element)
    {
        return;
    }

    FDeferredPaint& NewPaint = DeferredPaints.Emplace();
    NewPaint.Element  = Element;
    NewPaint.Geometry = Geometry;
}

void FWindow::QueueDeferredPainting(const FOnDeferredPaint& OnPaint) const
{
    if (!OnPaint.IsBound())
    {
        return;
    }

    FDeferredPaint& NewPaint = DeferredPaints.Emplace();
    NewPaint.OnPaint = OnPaint;
}

int32 FWindow::PaintDeferred(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    int32 MaxLayerId = LayerId;

    for (int32 Index = 0; Index < DeferredPaints.Size(); ++Index)
    {
        const FDeferredPaint& Paint = DeferredPaints[Index];
        if (Paint.Element)
        {
            MaxLayerId = Paint.Element->OnDraw(Paint.Geometry, OutCommandList, MaxLayerId + 1);
        }
        else if (Paint.OnPaint.IsBound())
        {
            MaxLayerId = Paint.OnPaint.Execute(OutCommandList, MaxLayerId + 1);
        }
    }

    DeferredPaints.Clear();
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

    if (MenuHost)
    {
        OutChildren.Add(MenuHost);
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

            const bool bIsCoveredByMenu = MenuHost && MenuHost->IsVisible() && MenuHost->CoversPoint(ClientPosition);
            const bool bIsOverlayModal  = Overlay && Overlay->IsVisible() && Overlay->CapturesAllInput();

            if (Content && !bIsOverlayModal && !bIsCoveredByMenu)
            {
                Content->FindChildrenContainingPoint(ClientPosition, OutParentElements);
            }

            if (Overlay && !bIsCoveredByMenu)
            {
                Overlay->FindChildrenContainingPoint(ClientPosition, OutParentElements);
            }

            if (MenuHost)
            {
                MenuHost->FindChildrenContainingPoint(ClientPosition, OutParentElements);
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

        bLayoutIsStale = true;

        if (PlatformWindow)
        {
            bCachedIsMaximized = PlatformWindow->IsMaximized();
        }

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
    if (CachedSize != InSize)
    {
        CachedSize = InSize;
        InvalidateDesiredSize();
    }
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

TSharedPtr<FMenuHost> FWindow::GetMenuHost() const
{
    return MenuHost;
}

TSharedPtr<FMenuHost> FWindow::GetOrCreateMenuHost()
{
    if (!MenuHost)
    {
        MenuHost = FMenuHost::Create();
        MenuHost->SetParentElement(AsWeakPtr());

        const IntVector2 Size = GetSize();
        MenuHost->Tick(FRectangle(IntVector2(), Size.X, Size.Y));
    }

    return MenuHost;
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
    if (PlatformWindow && PlatformWindow->IsValid())
    {
        return PlatformWindow->IsMaximized();
    }

    return bCachedIsMaximized;
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

        if (CachedOpacity < 1.0f)
        {
            PlatformWindow->SetWindowOpacity(CachedOpacity);
        }
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
    CachedOpacity = Alpha;

    if (PlatformWindow)
    {
        PlatformWindow->SetWindowOpacity(Alpha);
    }
}

float FWindow::GetOpacity() const
{
    return CachedOpacity;
}
