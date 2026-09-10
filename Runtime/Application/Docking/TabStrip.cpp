#include "Application/Application.h"
#include "Application/Docking/DockNode.h"
#include "Application/Docking/TabStrip.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

// The space either side of a tab label, and the room the close cross takes after it
constexpr int32 TAB_HORIZONTAL_PADDING = 12;
constexpr int32 TAB_CLOSE_WIDTH        = 18;

// Half the side of the cross drawn in the close button
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
    const int32 CloseWidth = bIsClosable ? TAB_CLOSE_WIDTH : 0;

    return IntVector2(LabelWidth + CloseWidth + TAB_HORIZONTAL_PADDING * 2, FDockMetrics::TabStripHeight);
}

int32 FTab::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&   Style  = FUIStyle::GetDefault();
    const FRectangle& Bounds = AllottedGeometry.Bounds;

    if (Bounds.IsEmpty())
    {
        return LayerId;
    }

    const FFloatColor& Fill = bIsActive ? Style.Tab.FillActive : (IsHovered() ? Style.Tab.FillHovered : Style.Tab.Fill);
    OutCommandList.AddBox(LayerId, Bounds, Fill, FCornerRadii::Top(Style.Tab.CornerRadius));

    if (bIsActive)
    {
        const FRectangle StripBounds(IntVector2(Bounds.Position.X, Bounds.GetBottom() - Style.Tab.ActiveStripThickness),
            Bounds.Width, Style.Tab.ActiveStripThickness);

        OutCommandList.AddBox(LayerId + 1, StripBounds, Style.Tab.ActiveStrip);
    }

    if (Font)
    {
        const int32 LabelWidth  = Font->MeasureWidth(StringView(Label));
        const int32 LabelHeight = Font->GetLineHeight();

        const FRectangle LabelBounds(IntVector2(Bounds.Position.X + TAB_HORIZONTAL_PADDING, Bounds.Position.Y + (Bounds.Height - LabelHeight) / 2),
            LabelWidth, LabelHeight);

        OutCommandList.AddText(LayerId + 1, LabelBounds, Label, Font.Get(), Style.GetTextColor(GetInteractionState()));
    }

    if (bIsClosable)
    {
        const FRectangle CloseBounds = GetCloseButtonRectangle();

        if (bIsCloseHovered)
        {
            OutCommandList.AddBox(LayerId + 1, CloseBounds, Style.Tab.CloseHovered, FCornerRadii(Style.Tab.CornerRadius));
        }

        const float CenterX = static_cast<float>(CloseBounds.Position.X) + static_cast<float>(CloseBounds.Width) * 0.5f;
        const float CenterY = static_cast<float>(CloseBounds.Position.Y) + static_cast<float>(CloseBounds.Height) * 0.5f;

        const FFloatColor& CrossColor = (bIsActive || IsHovered()) ? Style.Colors.Text : Style.Colors.TextDisabled;

        OutCommandList.AddLine(LayerId + 2, Vector2(CenterX - TAB_CLOSE_EXTENT, CenterY - TAB_CLOSE_EXTENT),
            Vector2(CenterX + TAB_CLOSE_EXTENT, CenterY + TAB_CLOSE_EXTENT), CrossColor, TAB_CLOSE_THICKNESS);
        OutCommandList.AddLine(LayerId + 2, Vector2(CenterX + TAB_CLOSE_EXTENT, CenterY - TAB_CLOSE_EXTENT),
            Vector2(CenterX - TAB_CLOSE_EXTENT, CenterY + TAB_CLOSE_EXTENT), CrossColor, TAB_CLOSE_THICKNESS);
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

FRectangle FTab::GetCloseButtonRectangle() const
{
    if (!bIsClosable)
    {
        return FRectangle();
    }

    const FRectangle& Bounds = GetContentRectangle();
    const int32       Inset  = Math::Max((Bounds.Height - TAB_CLOSE_WIDTH) / 2, 0);

    return FRectangle(IntVector2(Bounds.GetRight() - TAB_HORIZONTAL_PADDING - TAB_CLOSE_WIDTH, Bounds.Position.Y + Inset),
        TAB_CLOSE_WIDTH, TAB_CLOSE_WIDTH);
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
    , Tabs()
    , ActivePanelId()
    , DraggedPanelId()
    , DetachedPanelId()
    , DragOrigin()
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
    bAllowReorder             = Desc.bAllowReorder;
    bAllowTearOut             = Desc.bAllowTearOut;
    OnTabActivatedDelegate    = Desc.OnTabActivated;
    OnTabClosedDelegate       = Desc.OnTabClosed;
    OnTabReorderedDelegate    = Desc.OnTabReordered;
    OnTabDragDetachedDelegate = Desc.OnTabDragDetached;
    OnTabDragMovedDelegate    = Desc.OnTabDragMoved;
    OnTabDragFinishedDelegate = Desc.OnTabDragFinished;
}

IntVector2 FTabStrip::ComputeDesiredSize() const
{
    const FUITabStyle& TabStyle = FUIStyle::GetDefault().Tab;

    IntVector2 DesiredSize(0, FDockMetrics::TabStripHeight);
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        DesiredSize.X += Tab->GetCachedDesiredSize().X + TabStyle.Spacing;
    }

    return DesiredSize;
}

void FTabStrip::OnArrange(const FRectangle& AllottedBounds)
{
    const FUITabStyle& TabStyle = FUIStyle::GetDefault().Tab;

    int32 Offset = AllottedBounds.Position.X + TabStyle.Spacing;
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        const int32 TabWidth = Tab->GetCachedDesiredSize().X;
        Tab->Tick(FRectangle(IntVector2(Offset, AllottedBounds.Position.Y + TabStyle.TopInset),
            TabWidth, Math::Max(AllottedBounds.Height - TabStyle.TopInset, 0)));

        Offset += TabWidth + TabStyle.Spacing;
    }
}

void FTabStrip::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        OutChildren.Add(Tab);
    }
}

int32 FTabStrip::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Tab.StripFill);

    int32 NextLayerId = LayerId + 1;
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        if (Tab->IsVisible())
        {
            const FDrawGeometry TabGeometry(Tab->GetContentRectangle(), AllottedGeometry.Scale);
            NextLayerId = Math::Max(NextLayerId, Tab->OnDraw(TabGeometry, OutCommandList, LayerId + 1));
        }
    }

    return NextLayerId;
}

void FTabStrip::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        if (Tab)
        {
            Tab->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }
    }
}

void FTabStrip::AddTab(const String& PanelId, const String& Label, bool bIsClosable)
{
    TSharedPtr<FTab> Tab = FTab::Create(PanelId, Label, Font, bIsClosable);
    Tab->SetOwner(this);
    Tab->SetParentElement(AsWeakPtr());

    Tabs.Add(Tab);

    if (ActivePanelId.IsEmpty())
    {
        SetActiveTab(PanelId);
    }
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
        Tabs.RemoveAt(Index);

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
}

void FTabStrip::ClearTabs()
{
    for (const TSharedPtr<FTab>& Tab : Tabs)
    {
        Tab->SetOwner(nullptr);
    }

    Tabs.Clear();
    ActivePanelId.Clear();
    DraggedPanelId.Clear();
    DetachedPanelId.Clear();
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
