#include "Application/Application.h"
#include "Application/Docking/TabStrip.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Elements/ScrollBar.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformTime.h"

constexpr float TAB_CLOSE_EXTENT    = 4.0f;
constexpr float TAB_CLOSE_THICKNESS = 1.5f;

TSharedPtr<FTab> FTab::Create(const String& InPanelId, const String& InLabel, const TSharedPtr<IFontFace>& InFont, bool bInIsClosable)
{
    TSharedPtr<FTab> NewTab = MakeSharedPtr<FTab>();
    NewTab->PanelId     = InPanelId;
    NewTab->Label       = InLabel;
    NewTab->Font        = InFont;
    NewTab->bIsClosable = bInIsClosable;
    return NewTab;
}

FTab::FTab()
    : FInteractiveElement()
    , PanelId()
    , Label()
    , Font(nullptr)
    , Style(FUIStyle::GetDefault().Tab)
    , CloseIcon()
    , OwnerStrip(nullptr)
    , bIsClosable(false)
    , bIsActive(false)
    , bIsCloseHovered(false)
{
}

FTab::~FTab() = default;

IntVector2 FTab::ComputeDesiredSize() const
{
    const int32 LabelWidth = Font ? Font->MeasureWidth(StringView(Label)) : 0;
    const int32 PillHeight = Math::Max(Style.StripHeight - Style.TopInset - Style.BottomInset, 0);

    const int32 TrailingWidth = bIsClosable
        ? Style.LabelCloseGap + Style.CloseSize + Style.CloseInset
        : Style.HorizontalPadding;

    const int32 PillWidth = Style.HorizontalPadding + LabelWidth + TrailingWidth;
    return IntVector2(Math::Max(PillWidth, Style.MinWidth), PillHeight);
}

int32 FTab::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyleColors& Colors = FUIStyle::GetDefault().Colors;
    const FRectangle&     Bounds = AllottedGeometry.Bounds;

    if (Bounds.IsEmpty())
    {
        return LayerId;
    }

    const FCornerRadii Radii(Style.CornerRadius);

    if (bIsActive || IsHovered())
    {
        const FFloatColor& Fill = bIsActive ? Style.FillActive : Style.FillHovered;
        OutCommandList.AddBox(LayerId, Bounds, Fill, Radii);
    }

    const int32 AccentThickness = bIsActive
        ? Math::Clamp(Style.ActiveStripThickness, 0, Bounds.Height)
        : 0;

    if (AccentThickness > 0)
    {
        OutCommandList.AddRoundedAccentRing(LayerId, Bounds, Radii, static_cast<float>(AccentThickness),
            Style.ActiveStrip, Style.ActiveStripFadeFraction, Style.ActiveStripTrailAlpha);
    }

    if (Style.SeparatorThickness > 0)
    {
        const FRectangle SeparatorBounds(IntVector2(Bounds.GetRight() - Style.SeparatorThickness, Bounds.Position.Y),
            Style.SeparatorThickness, Bounds.Height);

        OutCommandList.AddBox(LayerId, SeparatorBounds, Style.Separator);
    }

    if (Font)
    {
        const int32 LabelWidth = Font->MeasureWidth(StringView(Label));

        const int32 LabelRegionRight = bIsClosable
            ? GetCloseButtonRectangle().Position.X
            : Bounds.GetRight();

        const int32 LabelRegionWidth = Math::Max(LabelRegionRight - Bounds.Position.X, 0);
        const int32 LabelX           = Bounds.Position.X + Math::Max((LabelRegionWidth - LabelWidth) / 2, 0);
        const int32 LabelY           = Bounds.Position.Y + Style.LabelOffsetY;

        const FRectangle LabelBounds(IntVector2(LabelX, LabelY), LabelWidth, Math::Max(Bounds.Height - Style.LabelOffsetY, 0));

        const FFloatColor& LabelColor = bIsActive ? Colors.Text : Colors.TextDisabled;
        OutCommandList.AddText(LayerId + 1, LabelBounds, Label, Font.Get(), LabelColor);
    }

    if (bIsClosable && (bIsActive || IsHovered()))
    {
        const FRectangle CloseBounds = GetCloseButtonRectangle();

        if (bIsCloseHovered)
        {
            OutCommandList.AddBox(LayerId + 1, CloseBounds, Style.CloseHovered, FCornerRadii(Style.CloseCornerRadius));
        }

        const FFloatColor& CrossColor = Colors.Text;

        if (CloseIcon.IsValid())
        {
            const int32      Inset      = Math::Max((Style.CloseSize - Style.CloseIconSize) / 2, 0);
            const FRectangle IconBounds = CloseBounds.Deflate(FMargin(Inset));

            OutCommandList.AddImage(LayerId + 2, IconBounds, CloseIcon, CrossColor);
        }
        else
        {
            const float CenterX = static_cast<float>(CloseBounds.Position.X) + static_cast<float>(CloseBounds.Width) * 0.5f;
            const float CenterY = static_cast<float>(CloseBounds.Position.Y) + static_cast<float>(CloseBounds.Height) * 0.5f;

            OutCommandList.AddLine(LayerId + 2, Vector2(CenterX - TAB_CLOSE_EXTENT, CenterY - TAB_CLOSE_EXTENT),
                Vector2(CenterX + TAB_CLOSE_EXTENT, CenterY + TAB_CLOSE_EXTENT), CrossColor, TAB_CLOSE_THICKNESS);
            OutCommandList.AddLine(LayerId + 2, Vector2(CenterX + TAB_CLOSE_EXTENT, CenterY - TAB_CLOSE_EXTENT),
                Vector2(CenterX - TAB_CLOSE_EXTENT, CenterY + TAB_CLOSE_EXTENT), CrossColor, TAB_CLOSE_THICKNESS);
        }
    }

    return LayerId + 3;
}

FEventResponse FTab::OnMouseMove(const FCursorEvent& CursorEvent)
{
    bIsCloseHovered = bIsClosable && GetCloseButtonRectangle().EncapsulatesPoint(CursorEvent.GetClientPosition());
    return FInteractiveElement::OnMouseMove(CursorEvent);
}

FEventResponse FTab::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    bIsCloseHovered = false;
    return FInteractiveElement::OnMouseLeft(CursorEvent);
}

FEventResponse FTab::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    const FEventResponse Response = FInteractiveElement::OnMouseButtonDown(CursorEvent);

    if (Response.IsEventHandled() && OwnerStrip)
    {
        OwnerStrip->OnTabPressed(this, CursorEvent.GetClientPosition());
    }

    return Response;
}

FEventResponse FTab::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    const bool bWasPressed = IsPressed();

    const FEventResponse Response = FInteractiveElement::OnMouseButtonUp(CursorEvent);
    if (bWasPressed && OwnerStrip)
    {
        OwnerStrip->OnTabReleased(this, CursorEvent.GetClientPosition(), CursorEvent.GetScreenPosition());
    }

    return Response;
}

void FTab::SetOwner(FTabStrip* InOwnerStrip)
{
    OwnerStrip = InOwnerStrip;
}

void FTab::SetActive(bool bInIsActive)
{
    bIsActive = bInIsActive;
}

void FTab::SetStyle(const FUITabStyle& InStyle)
{
    Style = InStyle;
    InvalidateDesiredSize();
}

void FTab::SetCloseIcon(const FUIBrush& InCloseIcon)
{
    CloseIcon = InCloseIcon;
}

FRectangle FTab::GetCloseButtonRectangle() const
{
    if (!bIsClosable)
    {
        return FRectangle();
    }

    const FRectangle& Bounds = GetContentRectangle();
    const int32       Inset  = Math::Max((Bounds.Height - Style.CloseSize) / 2, 0);

    return FRectangle(IntVector2(Bounds.GetRight() - Style.CloseInset - Style.CloseSize, Bounds.Position.Y + Inset),
        Style.CloseSize, Style.CloseSize);
}

void FTab::OnDragged(const FCursorEvent& CursorEvent)
{
    if (OwnerStrip)
    {
        OwnerStrip->OnTabDragged(this, CursorEvent.GetClientPosition(), CursorEvent.GetScreenPosition());
    }
}

TSharedPtr<FTabStrip> FTabStrip::Create(const FDesc& Desc)
{
    TSharedPtr<FTabStrip> NewStrip = MakeSharedPtr<FTabStrip>();
    NewStrip->Initialize(Desc);
    return NewStrip;
}

FTabStrip::FTabStrip()
    : FVisualElement()
    , Font(nullptr)
    , Style(FUIStyle::GetDefault().Tab)
    , CloseIcon()
    , Tabs()
    , ScrollBar(nullptr)
    , ActivePanelId()
    , DraggedPanelId()
    , DetachedPanelId()
    , DragOrigin()
    , ScrollOffset(0)
    , ScrollAmountPerWheelStep(DefaultScrollAmountPerWheelStep)
    , ContentWidth(0)
    , ViewWidth(0)
    , FadeStartCounter(FPlatformTime::QueryPerformanceCounter())
    , FadeStartOpacity(0.0f)
    , ScrollBarOpacity(0.0f)
    , bIsCursorOver(false)
    , bAllowReorder(true)
    , bAllowTearOut(true)
    , OnTabActivatedDelegate()
    , OnTabClosedDelegate()
    , OnTabReorderedDelegate()
    , OnTabDragDetachedDelegate()
    , OnTabDragMovedDelegate()
    , OnTabDragFinishedDelegate()
{
}

FTabStrip::~FTabStrip() = default;

void FTabStrip::Initialize(const FDesc& Desc)
{
    Font                      = Desc.Font;
    Style                     = Desc.Style;
    CloseIcon                 = Desc.CloseIcon;
    bAllowReorder             = Desc.bAllowReorder;
    bAllowTearOut             = Desc.bAllowTearOut;
    OnTabActivatedDelegate    = Desc.OnTabActivated;
    OnTabClosedDelegate       = Desc.OnTabClosed;
    OnTabReorderedDelegate    = Desc.OnTabReordered;
    OnTabDragDetachedDelegate = Desc.OnTabDragDetached;
    OnTabDragMovedDelegate    = Desc.OnTabDragMoved;
    OnTabDragFinishedDelegate = Desc.OnTabDragFinished;

    FUIScrollBarStyle BarStyle = FUIStyle::GetDefault().ScrollBar;
    BarStyle.Track.A   = 0.0f;
    BarStyle.CornerRadius = static_cast<float>(Style.ScrollBarThickness) * 0.5f;

    FScrollBar::FDesc BarDesc;
    BarDesc.Orientation     = EOrientation::Horizontal;
    BarDesc.Thickness       = Style.ScrollBarThickness;
    BarDesc.MinThumbLength  = 24;
    BarDesc.TrackPadding    = FMargin(0);
    BarDesc.Style           = BarStyle;
    BarDesc.OnOffsetChanged = FOnScrollBarOffsetChanged::CreateRaw(this, &FTabStrip::SetScrollOffset);

    ScrollBar = FScrollBar::Create(BarDesc);
    ScrollBar->SetParentElement(AsWeakPtr());
    ScrollBar->SetOpacity(0.0f);
}

IntVector2 FTabStrip::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(Tabs.IsEmpty() ? 0 : Style.Spacing, Style.StripHeight);
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        DesiredSize.X += Tab->GetCachedDesiredSize().X + Style.Spacing;
    }

    return DesiredSize;
}

void FTabStrip::OnArrange(const FRectangle& AllottedBounds)
{
    ContentWidth = ComputeDesiredSize().X;
    ViewWidth    = AllottedBounds.Width;
    ScrollOffset = Math::Clamp(ScrollOffset, 0, GetMaxScrollOffset());

    const int32 BarBand = GetMaxScrollOffset() > 0
        ? Math::Max((Style.ScrollBarThickness + Style.ScrollBarGap) - Style.BottomInset, 0)
        : 0;

    const int32 TabHeight = Math::Max(AllottedBounds.Height - Style.TopInset - Style.BottomInset - BarBand, 0);

    int32 Offset = (AllottedBounds.Position.X + Style.Spacing) - ScrollOffset;
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        const int32 TabWidth = Tab->GetCachedDesiredSize().X;
        Tab->Tick(FRectangle(IntVector2(Offset, AllottedBounds.Position.Y + Style.TopInset), TabWidth, TabHeight));

        Offset += TabWidth + Style.Spacing;
    }

    UpdateScrollBar(AllottedBounds);
}

void FTabStrip::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        OutChildren.Add(Tab);
    }

    if (ScrollBar)
    {
        OutChildren.Add(ScrollBar);
    }
}

int32 FTabStrip::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.StripFill);

    OutCommandList.PushClip(LayerId, AllottedGeometry.Bounds);

    int32 NextLayerId = LayerId + 1;
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        if (Tab->IsVisible())
        {
            const FDrawGeometry TabGeometry(Tab->GetContentRectangle(), AllottedGeometry.Scale);
            NextLayerId = Math::Max(NextLayerId, Tab->OnDraw(TabGeometry, OutCommandList, LayerId + 1));
        }
    }

    if (ScrollBar && ScrollBar->GetOpacity() > 0.0f && GetMaxScrollOffset() > 0)
    {
        const FDrawGeometry BarGeometry(ScrollBar->GetContentRectangle(), AllottedGeometry.Scale);
        NextLayerId = ScrollBar->OnDraw(BarGeometry, OutCommandList, NextLayerId + 1);
    }

    OutCommandList.PopClip(NextLayerId);

    return NextLayerId;
}

void FTabStrip::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (!GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        return;
    }

    if (ScrollBar && GetMaxScrollOffset() > 0 && ScrollBar->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        ScrollBar->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        return;
    }

    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        if (Tab)
        {
            Tab->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }
    }
}

FEventResponse FTabStrip::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (GetMaxScrollOffset() <= 0)
    {
        return FEventResponse::Unhandled();
    }

    const int32 Delta = Math::RoundToInt(CursorEvent.GetScrollDelta() * static_cast<float>(ScrollAmountPerWheelStep));
    SetScrollOffset(ScrollOffset - Delta);

    return FEventResponse::Handled();
}

FEventResponse FTabStrip::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    SetCursorOver(true);
    return FVisualElement::OnMouseEntered(CursorEvent);
}

FEventResponse FTabStrip::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    SetCursorOver(false);
    return FVisualElement::OnMouseLeft(CursorEvent);
}

void FTabStrip::SetScrollOffset(int32 InScrollOffset)
{
    ScrollOffset = Math::Clamp(InScrollOffset, 0, GetMaxScrollOffset());
}

int32 FTabStrip::GetMaxScrollOffset() const
{
    return Math::Max(0, ContentWidth - ViewWidth);
}

void FTabStrip::ScrollTabIntoView(const String& PanelId)
{
    int32 Offset = Style.Spacing;
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        const int32 TabWidth = Tab->GetCachedDesiredSize().X;
        if (Tab->GetPanelId() == PanelId)
        {
            if (Offset < ScrollOffset)
            {
                ScrollOffset = Offset;
            }
            else if ((Offset + TabWidth) > (ScrollOffset + ViewWidth))
            {
                ScrollOffset = (Offset + TabWidth) - ViewWidth;
            }

            ScrollOffset = Math::Clamp(ScrollOffset, 0, GetMaxScrollOffset());
            return;
        }

        Offset += TabWidth + Style.Spacing;
    }
}

void FTabStrip::SetCursorOver(bool bInIsCursorOver)
{
    if (bIsCursorOver == bInIsCursorOver)
    {
        return;
    }

    bIsCursorOver    = bInIsCursorOver;
    FadeStartOpacity = ScrollBarOpacity;
    FadeStartCounter = FPlatformTime::QueryPerformanceCounter();
}

double FTabStrip::GetSecondsSinceFadeStart() const
{
    const uint64 Now = FPlatformTime::QueryPerformanceCounter();
    return static_cast<double>(Now - FadeStartCounter) / static_cast<double>(FPlatformTime::QueryPerformanceFrequency());
}

void FTabStrip::UpdateScrollBar(const FRectangle& AllottedBounds)
{
    if (!ScrollBar)
    {
        return;
    }

    const float Target   = bIsCursorOver ? 1.0f : 0.0f;
    const float Duration = bIsCursorOver ? Style.ScrollBarFadeInDuration : Style.ScrollBarFadeOutDuration;

    if (Duration > 0.0f)
    {
        const float Progress = Math::Clamp(static_cast<float>(GetSecondsSinceFadeStart()) / Duration, 0.0f, 1.0f);
        ScrollBarOpacity     = Math::Lerp(FadeStartOpacity, Target, Progress);
    }
    else
    {
        ScrollBarOpacity = Target;
    }

    ScrollBar->SetOpacity(ScrollBarOpacity);
    ScrollBar->SetScrollState(ContentWidth, ViewWidth, ScrollOffset);

    const int32 BarTop = AllottedBounds.GetBottom() - Style.ScrollBarThickness;
    ScrollBar->Tick(FRectangle(IntVector2(AllottedBounds.Position.X, BarTop), AllottedBounds.Width, Style.ScrollBarThickness));
}

void FTabStrip::AddTab(const String& PanelId, const String& Label, bool bIsClosable)
{
    TSharedPtr<FTab> Tab = FTab::Create(PanelId, Label, Font, bIsClosable);
    Tab->SetStyle(Style);
    Tab->SetCloseIcon(CloseIcon);
    Tab->SetOwner(this);
    Tab->SetParentElement(AsWeakPtr());

    Tabs.Add(Tab);

    if (ActivePanelId.IsEmpty())
    {
        SetActiveTab(PanelId);
    }

    ScrollTabIntoView(PanelId);
}

void FTabStrip::RemoveTab(const String& PanelId)
{
    for (int32 Index = 0; Index < Tabs.Size(); ++Index)
    {
        if (Tabs[Index]->GetPanelId() != PanelId)
        {
            continue;
        }

        const bool bWasActive = Tabs[Index]->IsActive();
        Tabs[Index]->SetOwner(nullptr);
        Tabs[Index]->SetParentElement(TWeakPtr<FVisualElement>());
        Tabs.RemoveAt(Index);
        InvalidateDesiredSize();

        if (bWasActive)
        {
            ActivePanelId.Clear();

            if (!Tabs.IsEmpty())
            {
                SetActiveTab(Tabs[Math::Min(Index, Tabs.Size() - 1)]->GetPanelId());
            }
        }

        return;
    }
}

void FTabStrip::SetActiveTab(const String& PanelId)
{
    bool bFoundTab = false;
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        bFoundTab = bFoundTab || Tab->GetPanelId() == PanelId;
    }

    if (!bFoundTab)
    {
        return;
    }

    ActivePanelId = PanelId;
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        Tab->SetActive(Tab->GetPanelId() == PanelId);
    }

    ScrollTabIntoView(PanelId);
}

void FTabStrip::ClearTabs()
{
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        Tab->SetOwner(nullptr);
        Tab->SetParentElement(TWeakPtr<FVisualElement>());
    }

    Tabs.Clear();
    ActivePanelId.Clear();
    DraggedPanelId.Clear();
    DetachedPanelId.Clear();
    InvalidateDesiredSize();
}

void FTabStrip::OnTabPressed(FTab* Tab, const IntVector2& ClientPosition)
{
    if (!Tab)
    {
        return;
    }

    DraggedPanelId = Tab->GetPanelId();
    DragOrigin     = ClientPosition;

    DetachedPanelId.Clear();

    SetActiveTab(Tab->GetPanelId());
    OnTabActivatedDelegate.ExecuteIfBound(Tab->GetPanelId());
}

void FTabStrip::OnTabDragged(FTab* Tab, const IntVector2& ClientPosition, const IntVector2& ScreenPosition)
{
    if (!DetachedPanelId.IsEmpty())
    {
        OnTabDragMovedDelegate.ExecuteIfBound(DetachedPanelId, ClientPosition, ScreenPosition);
        return;
    }

    if (!Tab || DraggedPanelId.IsEmpty())
    {
        return;
    }

    const FRectangle& StripBounds = GetContentRectangle();

    const int32 DistanceAbove = StripBounds.Position.Y - ClientPosition.Y;
    const int32 DistanceBelow = ClientPosition.Y - StripBounds.GetBottom();
    const int32 DistanceOff   = Math::Max(DistanceAbove, DistanceBelow);

    if (bAllowTearOut && DistanceOff > TearOutDistance)
    {
        DetachedPanelId = DraggedPanelId;
        DraggedPanelId.Clear();

        OnTabDragDetachedDelegate.ExecuteIfBound(DetachedPanelId, ClientPosition, ScreenPosition);
        return;
    }

    if (!bAllowReorder)
    {
        return;
    }

    const int32 FromIndex = FindTabIndex(Tab);
    const int32 ToIndex   = FindTabIndexAt(ClientPosition.X);

    if (FromIndex >= 0 && ToIndex >= 0 && FromIndex != ToIndex)
    {
        MoveTab(FromIndex, ToIndex);
    }
}

void FTabStrip::OnTabReleased(FTab* Tab, const IntVector2& ClientPosition, const IntVector2& ScreenPosition)
{
    if (Tab && Tab->IsClosable() && DetachedPanelId.IsEmpty())
    {
        const FRectangle CloseBounds = Tab->GetCloseButtonRectangle();
        if (CloseBounds.EncapsulatesPoint(DragOrigin) && CloseBounds.EncapsulatesPoint(ClientPosition))
        {
            DraggedPanelId.Clear();
            OnTabClosedDelegate.ExecuteIfBound(Tab->GetPanelId());
            return;
        }
    }

    DraggedPanelId.Clear();

    if (DetachedPanelId.IsEmpty())
    {
        return;
    }

    const String FinishedPanelId = DetachedPanelId;
    DetachedPanelId.Clear();

    OnTabDragFinishedDelegate.ExecuteIfBound(FinishedPanelId, ClientPosition, ScreenPosition);
}

const String& FTabStrip::GetActivePanelId() const
{
    return ActivePanelId;
}

void FTabStrip::MoveTab(int32 FromIndex, int32 ToIndex)
{
    if (FromIndex < 0 || FromIndex >= Tabs.Size() || ToIndex < 0 || ToIndex >= Tabs.Size())
    {
        return;
    }

    TSharedPtr<FTab> Tab = Tabs[FromIndex];

    Tabs.RemoveAt(FromIndex);
    Tabs.Insert(ToIndex, Tab);

    OnArrange(GetContentRectangle());
    OnTabReorderedDelegate.ExecuteIfBound(Tab->GetPanelId(), ToIndex);
}

int32 FTabStrip::FindTabIndexAt(int32 PositionX) const
{
    for (int32 Index = 0; Index < Tabs.Size(); ++Index)
    {
        const FRectangle& Bounds = Tabs[Index]->GetContentRectangle();
        if (PositionX >= Bounds.Position.X && PositionX < Bounds.GetRight())
        {
            return Index;
        }
    }

    if (Tabs.IsEmpty())
    {
        return -1;
    }

    return PositionX < Tabs[0]->GetContentRectangle().Position.X ? 0 : Tabs.Size() - 1;
}

int32 FTabStrip::FindTabIndex(const FTab* Tab) const
{
    for (int32 Index = 0; Index < Tabs.Size(); ++Index)
    {
        if (Tabs[Index].Get() == Tab)
        {
            return Index;
        }
    }

    return -1;
}
