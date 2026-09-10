#include "Application/Elements/TreeView.h"
#include "Application/Application.h"
#include "Application/ElementPath.h"
#include "Application/Elements/ScrollBar.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Text/TextLayout.h"
#include "Core/Math/Math.h"

constexpr int32 TREE_DISCLOSURE_SIZE    = 8;
constexpr int32 TREE_DISCLOSURE_SPACING = 4;

constexpr int32 TREE_ICON_SIZE    = 16;
constexpr int32 TREE_ICON_SPACING = 4;

constexpr int32 TREE_HEADER_PADDING = 4;

constexpr float TREE_HOVER_OPACITY = 0.5f;

static void DrawDisclosureTriangle(FDrawCommandList& OutCommandList, int32 LayerId, const FRectangle& Bounds, bool bIsExpanded, const FFloatColor& Tint)
{
    const float Left   = static_cast<float>(Bounds.Position.X);
    const float Top    = static_cast<float>(Bounds.Position.Y);
    const float Right  = static_cast<float>(Bounds.GetRight());
    const float Bottom = static_cast<float>(Bounds.GetBottom());

    Vector2 Corners[3];
    if (bIsExpanded)
    {
        const float CenterX = (Left + Right) * 0.5f;
        Corners[0] = Vector2(Left, Top);
        Corners[1] = Vector2(Right, Top);
        Corners[2] = Vector2(CenterX, Bottom);
    }
    else
    {
        const float CenterY = (Top + Bottom) * 0.5f;
        Corners[0] = Vector2(Left, Top);
        Corners[1] = Vector2(Right, CenterY);
        Corners[2] = Vector2(Left, Bottom);
    }

    OutCommandList.AddConvexPolygon(LayerId, TArrayView<const Vector2>(Corners, 3), Tint);
}

TSharedPtr<FTreeItem> FTreeItem::Create(const String& InLabel, void* InUserData)
{
    TSharedPtr<FTreeItem> NewItem = MakeSharedPtr<FTreeItem>();
    NewItem->Label    = InLabel;
    NewItem->UserData = InUserData;
    return NewItem;
}

void FTreeItem::AddChild(const TSharedPtr<FTreeItem>& Child)
{
    if (!Child)
    {
        return;
    }

    Child->Parent = AsWeakPtr();
    Children.Add(Child);
}

void FTreeItem::ClearChildren()
{
    Children.Clear();
}

bool FTreeItem::HasChildren() const
{
    return !Children.IsEmpty();
}

int32 FTreeItem::GetDepth() const
{
    int32 Depth = 0;

    TWeakPtr<FTreeItem> Current = Parent;
    while (Current.IsValid())
    {
        ++Depth;
        Current = Current->Parent;
    }

    return Depth;
}

TSharedPtr<FTreeView> FTreeView::Create(const FDesc& Desc)
{
    TSharedPtr<FTreeView> NewTreeView = MakeSharedPtr<FTreeView>();
    NewTreeView->Initialize(Desc);
    return NewTreeView;
}

FTreeView::FTreeView()
    : FVisualElement()
    , RootItems()
    , Selection()
    , VisibleRows()
    , Font(nullptr)
    , ScrollBar(nullptr)
    , Style()
    , ExpandedArrow()
    , CollapsedArrow()
    , FilterText()
    , LabelColumnHeader()
    , TypeColumnHeader()
    , PressPosition()
    , ArrowSize(16)
    , TypeColumnWidth(0)
    , HeaderHeight(FUIStyle::GetDefault().Metrics.RowHeight)
    , RowHeight(FUIStyle::GetDefault().Metrics.RowHeight)
    , IndentPerLevel(FUIStyle::GetDefault().TreeRow.IndentPerLevel)
    , ScrollOffset(0)
    , ViewHeight(0)
    , AnchorRowIndex(InvalidRowIndex)
    , HoveredRowIndex(InvalidRowIndex)
    , PressedRowIndex(InvalidRowIndex)
    , bAlternateRowColors(false)
    , bHighlightAncestors(false)
    , bAllowMultiSelect(true)
    , bRowsDirty(true)
    , bReserveIconColumn(false)
    , OnSelectionChangedDelegate()
    , OnItemActivatedDelegate()
    , OnExpansionChangedDelegate()
    , OnDragDetectedDelegate()
{
}

FTreeView::~FTreeView() = default;

void FTreeView::Initialize(const FDesc& Desc)
{
    Font                       = Desc.Font;
    Style                      = Desc.Style;
    ExpandedArrow              = Desc.ExpandedArrow;
    CollapsedArrow             = Desc.CollapsedArrow;
    ArrowSize                  = Math::Max(1, Desc.ArrowSize);
    TypeColumnWidth            = Math::Max(0, Desc.TypeColumnWidth);
    LabelColumnHeader          = Desc.LabelColumnHeader;
    TypeColumnHeader           = Desc.TypeColumnHeader;
    HeaderHeight               = Math::Max(0, Desc.HeaderHeight);
    RowHeight                  = Math::Max(1, Desc.RowHeight);
    IndentPerLevel             = Math::Max(0, Desc.IndentPerLevel);
    bAlternateRowColors        = Desc.bAlternateRowColors;
    bHighlightAncestors        = Desc.bHighlightAncestors;
    bAllowMultiSelect          = Desc.bAllowMultiSelect;
    OnSelectionChangedDelegate = Desc.OnSelectionChanged;
    OnItemActivatedDelegate    = Desc.OnItemActivated;
    OnExpansionChangedDelegate = Desc.OnExpansionChanged;
    OnDragDetectedDelegate     = Desc.OnDragDetected;

    if (Desc.bShowScrollBar)
    {
        FScrollBar::FDesc BarDesc;
        BarDesc.Orientation     = EOrientation::Vertical;
        BarDesc.OnOffsetChanged = FOnScrollBarOffsetChanged::CreateRaw(this, &FTreeView::OnScrollBarMoved);

        ScrollBar = FScrollBar::Create(BarDesc);
        if (ScrollBar)
        {
            ScrollBar->SetParentElement(AsWeakPtr());
        }
    }
}

IntVector2 FTreeView::ComputeDesiredSize() const
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = GetVisibleRows();

    int32 Width = 0;
    for (const TSharedPtr<FTreeItem>& Row : Rows)
    {
        Width = Math::Max(Width, ComputeRowExtent(Row));
    }

    return IntVector2(Width, GetHeaderExtent() + (Rows.Size() * RowHeight));
}

void FTreeView::OnArrange(const FRectangle& AllottedBounds)
{
    RebuildVisibleRows();

    ViewHeight   = AllottedBounds.Height;
    ScrollOffset = Math::Clamp(ScrollOffset, 0, GetMaxScrollOffset());

    if (ScrollBar)
    {
        const int32 HeaderExtent = GetHeaderExtent();
        const int32 Thickness    = FUIStyle::GetDefault().Metrics.ScrollBarThickness;

        ScrollBar->SetScrollState(GetVisibleRows().Size() * RowHeight, ViewHeight - HeaderExtent, ScrollOffset);
        ScrollBar->Tick(FRectangle(IntVector2(AllottedBounds.GetRight() - Thickness, AllottedBounds.Position.Y + HeaderExtent),
            Thickness, Math::Max(AllottedBounds.Height - HeaderExtent, 0)));
    }
}

void FTreeView::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (ScrollBar)
    {
        OutChildren.Add(ScrollBar);
    }
}

void FTreeView::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (ScrollBar && ScrollBar->IsScrollable() && ScrollBar->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        ScrollBar->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

int32 FTreeView::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle& Bounds = AllottedGeometry.Bounds;

    const TArray<TSharedPtr<FTreeItem>>& Rows        = GetVisibleRows();
    const int32                          TypeColumnX = Bounds.GetRight() - TypeColumnWidth;

    const int32 HeaderExtent = GetHeaderExtent();
    if (HeaderExtent > 0)
    {
        const FRectangle HeaderBounds(Bounds.Position, Bounds.Width, HeaderExtent);
        OutCommandList.AddBox(LayerId, HeaderBounds, Style.HeaderFill);

        if (Font)
        {
            const int32 CaptionY = Bounds.Position.Y + Font->GetTextBandOffset(HeaderExtent);
            const int32 LabelX   = Bounds.Position.X + TREE_HEADER_PADDING;

            OutCommandList.AddText(LayerId + 1,
                FRectangle(IntVector2(LabelX, CaptionY), Math::Max(TypeColumnX - LabelX, 0), Font->GetTextBandHeight()),
                LabelColumnHeader, Font.Get(), Style.SecondaryText);

            if (TypeColumnWidth > 0 && !TypeColumnHeader.IsEmpty())
            {
                OutCommandList.AddText(LayerId + 1,
                    FRectangle(IntVector2(TypeColumnX, CaptionY), TypeColumnWidth, Font->GetTextBandHeight()),
                    TypeColumnHeader, Font.Get(), Style.SecondaryText);
            }
        }

        const float SeparatorY = static_cast<float>(HeaderBounds.GetBottom());
        OutCommandList.AddLine(LayerId + 1, Vector2(static_cast<float>(Bounds.Position.X), SeparatorY),
            Vector2(static_cast<float>(Bounds.GetRight()), SeparatorY), Style.HeaderSeparator, 1.0f);
    }

    OutCommandList.PushClip(LayerId, FRectangle(IntVector2(Bounds.Position.X, Bounds.Position.Y + HeaderExtent),
        Bounds.Width, Math::Max(Bounds.Height - HeaderExtent, 0)));

    const bool bHasFocus = FApplication::IsInitialized() && FApplication::Get().GetFocusElementLeaf().Get() == this;

    FFloatColor HoverFill = Style.HoveredFill;
    HoverFill.A           = TREE_HOVER_OPACITY;

    const int32 FirstRow = Math::Clamp(ScrollOffset / RowHeight, 0, Math::Max(Rows.Size() - 1, 0));
    const int32 LastRow  = Math::Min(Rows.Size() - 1, (ScrollOffset + Bounds.Height) / RowHeight);

    for (int32 RowIndex = FirstRow; RowIndex <= LastRow; ++RowIndex)
    {
        const TSharedPtr<FTreeItem>& Item      = Rows[RowIndex];
        const FRectangle             RowBounds = ComputeRowBounds(Bounds, RowIndex);
        const int32                  Depth     = Item->GetDepth();

        if (bAlternateRowColors)
        {
            OutCommandList.AddBox(LayerId, RowBounds, (RowIndex % 2) == 1 ? Style.AlternateFill : Style.Fill);
        }

        if (IsSelected(Item))
        {
            OutCommandList.AddBox(LayerId, RowBounds, bHasFocus ? Style.SelectedFill : Style.InactiveSelectedFill);
        }
        else if (bHighlightAncestors && IsAncestorOfSelection(Item))
        {
            OutCommandList.AddBox(LayerId, RowBounds, Style.AncestorFill);
        }

        if (RowIndex == HoveredRowIndex)
        {
            OutCommandList.AddBox(LayerId, RowBounds, HoverFill);
        }

        const FRectangle DisclosureBounds = ComputeDisclosureBounds(RowBounds, Depth);
        if (Item->HasChildren())
        {
            const FUIBrush& ArrowBrush = Item->bIsExpanded ? ExpandedArrow : CollapsedArrow;
            if (ArrowBrush.IsValid())
            {
                OutCommandList.AddImage(LayerId + 1, DisclosureBounds, ArrowBrush, Style.ArrowTint);
            }
            else
            {
                DrawDisclosureTriangle(OutCommandList, LayerId + 1, DisclosureBounds, Item->bIsExpanded, Style.ArrowTint);
            }
        }

        const int32 PenX = ComputeLabelStartX(RowBounds, Item);

        const FUIBrush& Icon = (Item->bIsExpanded && Item->ExpandedIcon.IsValid()) ? Item->ExpandedIcon : Item->Icon;
        if (Icon.IsValid())
        {
            const IntVector2 IconPosition(DisclosureBounds.GetRight() + TREE_DISCLOSURE_SPACING, RowBounds.Position.Y + ((RowHeight - TREE_ICON_SIZE) / 2));
            OutCommandList.AddImage(LayerId + 1, FRectangle(IconPosition, TREE_ICON_SIZE, TREE_ICON_SIZE), Icon, Style.LabelText);
        }

        if (Font && !Item->Label.IsEmpty())
        {
            const int32      LabelRight = TypeColumnWidth > 0 ? Math::Min(TypeColumnX, RowBounds.GetRight()) : RowBounds.GetRight();
            const IntVector2 LabelPosition(PenX, RowBounds.Position.Y + Font->GetTextBandOffset(RowHeight));
            const FRectangle LabelBounds(LabelPosition, Math::Max(LabelRight - PenX, 0), Font->GetTextBandHeight());

            const int32 MatchOffset = FilterText.IsEmpty() ? String::InvalidIndex : Item->Label.Find(FilterText, EStringCaseType::NoCase);
            if (MatchOffset == String::InvalidIndex)
            {
                OutCommandList.AddText(LayerId + 2, LabelBounds, Item->Label, Font.Get(), Style.LabelText);
            }
            else
            {
                DrawTextWithSearchHighlight(OutCommandList, LayerId + 1, LabelBounds, Item->Label, MatchOffset, FilterText.Length(),
                    Font.Get(), Style.LabelText);
            }
        }

        if (Font && TypeColumnWidth > 0 && !Item->TypeLabel.IsEmpty())
        {
            const IntVector2 TypePosition(TypeColumnX, RowBounds.Position.Y + Font->GetTextBandOffset(RowHeight));
            const FRectangle TypeBounds(TypePosition, TypeColumnWidth, Font->GetTextBandHeight());

            OutCommandList.AddText(LayerId + 2, TypeBounds, Item->TypeLabel, Font.Get(), Style.SecondaryText);
        }
    }

    OutCommandList.PopClip(LayerId + 2);

    int32 MaxLayerId = LayerId + 2;
    if (ScrollBar && ScrollBar->IsScrollable())
    {
        const FDrawGeometry BarGeometry(ScrollBar->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = ScrollBar->OnDraw(BarGeometry, OutCommandList, MaxLayerId + 1);
    }

    return MaxLayerId;
}

FEventResponse FTreeView::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const IntVector2 ClientPosition = CursorEvent.GetClientPosition();
    const int32      RowIndex       = FindRowAt(ClientPosition);
    if (RowIndex == InvalidRowIndex)
    {
        return FEventResponse::Unhandled();
    }

    const TSharedPtr<FTreeItem> Item      = GetVisibleRows()[RowIndex];
    const FRectangle            RowBounds = ComputeRowBounds(GetContentRectangle(), RowIndex);

    if (Item->HasChildren() && ComputeDisclosureBounds(RowBounds, Item->GetDepth()).EncapsulatesPoint(ClientPosition))
    {
        SetItemExpanded(Item, !Item->bIsExpanded);
        return FEventResponse::Handled();
    }

    PressedRowIndex = RowIndex;
    PressPosition   = ClientPosition;

    ApplySelectionFromClick(RowIndex, CursorEvent.GetModifierKeys());
    return FEventResponse::Handled();
}

FEventResponse FTreeView::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() == Keys::MouseButtonLeft)
    {
        PressedRowIndex = InvalidRowIndex;
    }

    if (CursorEvent.GetKey() != Keys::MouseButtonLeft || FindRowAt(CursorEvent.GetClientPosition()) == InvalidRowIndex)
    {
        return FEventResponse::Unhandled();
    }

    return FEventResponse::Handled();
}

FEventResponse FTreeView::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const int32 RowIndex = FindRowAt(CursorEvent.GetClientPosition());
    if (RowIndex == InvalidRowIndex)
    {
        return FEventResponse::Unhandled();
    }

    OnItemActivatedDelegate.ExecuteIfBound(GetVisibleRows()[RowIndex]);
    return FEventResponse::Handled();
}

FEventResponse FTreeView::OnMouseMove(const FCursorEvent& CursorEvent)
{
    const IntVector2 ClientPosition = CursorEvent.GetClientPosition();

    HoveredRowIndex = FindRowAt(ClientPosition);

    if (PressedRowIndex != InvalidRowIndex && OnDragDetectedDelegate.IsBound())
    {
        const IntVector2 Travel = ClientPosition - PressPosition;
        if ((Math::Abs(Travel.X) >= DragThreshold) || (Math::Abs(Travel.Y) >= DragThreshold))
        {
            const TArray<TSharedPtr<FTreeItem>>& Rows = GetVisibleRows();
            const TSharedPtr<FTreeItem>          Item = PressedRowIndex < Rows.Size() ? Rows[PressedRowIndex] : nullptr;

            PressedRowIndex = InvalidRowIndex;

            if (Item)
            {
                OnDragDetectedDelegate.Execute(Item, CursorEvent);
            }
        }
    }

    return FEventResponse::Unhandled();
}

FEventResponse FTreeView::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetScrollAxis() != EScrollAxis::Vertical)
    {
        return FEventResponse::Unhandled();
    }

    const int32 MaxScrollOffset = GetMaxScrollOffset();
    if (MaxScrollOffset <= 0)
    {
        return FEventResponse::Unhandled();
    }

    const int32 Delta = static_cast<int32>(CursorEvent.GetScrollDelta() * static_cast<float>(RowsPerWheelStep * RowHeight));
    ScrollOffset = Math::Clamp(ScrollOffset - Delta, 0, MaxScrollOffset);

    if (ScrollBar)
    {
        ScrollBar->SetOffset(ScrollOffset);
    }

    return FEventResponse::Handled();
}

FEventResponse FTreeView::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    HoveredRowIndex = InvalidRowIndex;
    PressedRowIndex = InvalidRowIndex;
    return FEventResponse::Unhandled();
}

FEventResponse FTreeView::OnKeyDown(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();
    if (Key == Keys::Up)
    {
        MoveSelection(-1);
    }
    else if (Key == Keys::Down)
    {
        MoveSelection(1);
    }
    else if (Key == Keys::Left)
    {
        CollapseOrMoveToParent();
    }
    else if (Key == Keys::Right)
    {
        ExpandOrMoveToFirstChild();
    }
    else if (Key == Keys::Enter || Key == Keys::KeypadEnter)
    {
        const int32 RowIndex = GetCurrentRowIndex();
        if (RowIndex == InvalidRowIndex)
        {
            return FEventResponse::Unhandled();
        }

        OnItemActivatedDelegate.ExecuteIfBound(GetVisibleRows()[RowIndex]);
    }
    else
    {
        return FEventResponse::Unhandled();
    }

    return FEventResponse::Handled();
}

bool FTreeView::SupportsKeyboardFocus() const
{
    return true;
}

void FTreeView::SetRootItems(const TArray<TSharedPtr<FTreeItem>>& InRoots)
{
    RootItems       = InRoots;
    AnchorRowIndex  = InvalidRowIndex;
    HoveredRowIndex = InvalidRowIndex;
    PressedRowIndex = InvalidRowIndex;
    bRowsDirty      = true;
}

void FTreeView::RequestRefresh()
{
    bRowsDirty = true;
}

const TArray<TSharedPtr<FTreeItem>>& FTreeView::GetVisibleRows() const
{
    RebuildVisibleRows();
    return VisibleRows;
}

void FTreeView::SetSelection(const TArray<TSharedPtr<FTreeItem>>& InSelection)
{
    Selection      = InSelection;
    AnchorRowIndex = InvalidRowIndex;
}

void FTreeView::ClearSelection()
{
    Selection.Clear();
    AnchorRowIndex = InvalidRowIndex;
}

bool FTreeView::IsSelected(const TSharedPtr<FTreeItem>& Item) const
{
    return Selection.Contains(Item);
}

void FTreeView::SetItemExpanded(const TSharedPtr<FTreeItem>& Item, bool bExpanded)
{
    if (!Item || Item->bIsExpanded == bExpanded)
    {
        return;
    }

    Item->bIsExpanded = bExpanded;
    bRowsDirty        = true;

    OnExpansionChangedDelegate.ExecuteIfBound(Item, bExpanded);
}

void FTreeView::ExpandAll()
{
    SetSubtreeExpanded(RootItems, true);
    bRowsDirty = true;
}

void FTreeView::CollapseAll()
{
    SetSubtreeExpanded(RootItems, false);
    bRowsDirty = true;
}

void FTreeView::SetFilterText(const String& InFilter)
{
    if (FilterText == InFilter)
    {
        return;
    }

    FilterText = InFilter;
    bRowsDirty = true;
}

void FTreeView::ScrollToItem(const TSharedPtr<FTreeItem>& Item)
{
    const int32 RowIndex = FindRowIndex(Item);
    if (RowIndex != InvalidRowIndex)
    {
        ScrollRowIntoView(RowIndex);
    }
}

TSharedPtr<FTreeItem> FTreeView::FindItemAt(const IntVector2& ClientPosition) const
{
    const int32 RowIndex = FindRowAt(ClientPosition);
    return RowIndex == InvalidRowIndex ? nullptr : GetVisibleRows()[RowIndex];
}

FRectangle FTreeView::GetItemRowBounds(const TSharedPtr<FTreeItem>& Item) const
{
    const int32 RowIndex = FindRowIndex(Item);
    return RowIndex == InvalidRowIndex ? FRectangle() : ComputeRowBounds(GetContentRectangle(), RowIndex);
}

FRectangle FTreeView::GetItemLabelBounds(const TSharedPtr<FTreeItem>& Item) const
{
    const FRectangle RowBounds = GetItemRowBounds(Item);
    if (RowBounds.IsEmpty())
    {
        return FRectangle();
    }

    const int32 LabelStartX = ComputeLabelStartX(RowBounds, Item);
    return FRectangle(IntVector2(LabelStartX, RowBounds.Position.Y), Math::Max(RowBounds.GetRight() - LabelStartX, 0), RowBounds.Height);
}

void FTreeView::SetSubtreeExpanded(const TArray<TSharedPtr<FTreeItem>>& Items, bool bExpanded)
{
    for (const TSharedPtr<FTreeItem>& Item : Items)
    {
        if (Item)
        {
            Item->bIsExpanded = bExpanded;
            SetSubtreeExpanded(Item->Children, bExpanded);
        }
    }
}

void FTreeView::RebuildVisibleRows() const
{
    if (!bRowsDirty)
    {
        return;
    }

    bRowsDirty = false;

    VisibleRows.Clear();
    AppendVisibleRows(RootItems);

    bReserveIconColumn = false;
    for (const TSharedPtr<FTreeItem>& Row : VisibleRows)
    {
        if (Row->Icon.IsValid() || Row->ExpandedIcon.IsValid())
        {
            bReserveIconColumn = true;
            break;
        }
    }
}

void FTreeView::AppendVisibleRows(const TArray<TSharedPtr<FTreeItem>>& Items) const
{
    const bool bIsFiltered = !FilterText.IsEmpty();

    for (const TSharedPtr<FTreeItem>& Item : Items)
    {
        if (!Item || (bIsFiltered && !PassesFilter(Item)))
        {
            continue;
        }

        VisibleRows.Add(Item);

        if (!Item->HasChildren())
        {
            continue;
        }

        const bool bIsForcedOpen = bIsFiltered && !MatchesFilterText(Item);
        if (Item->bIsExpanded || bIsForcedOpen)
        {
            AppendVisibleRows(Item->Children);
        }
    }
}

bool FTreeView::MatchesFilterText(const TSharedPtr<FTreeItem>& Item) const
{
    if (Item->Label.Contains(FilterText, EStringCaseType::NoCase))
    {
        return true;
    }

    return !Item->TypeLabel.IsEmpty() && Item->TypeLabel.Contains(FilterText, EStringCaseType::NoCase);
}

bool FTreeView::PassesFilter(const TSharedPtr<FTreeItem>& Item) const
{
    if (MatchesFilterText(Item))
    {
        return true;
    }

    for (const TSharedPtr<FTreeItem>& Child : Item->Children)
    {
        if (Child && PassesFilter(Child))
        {
            return true;
        }
    }

    return false;
}

FRectangle FTreeView::ComputeRowBounds(const FRectangle& ViewBounds, int32 RowIndex) const
{
    const IntVector2 Position(ViewBounds.Position.X, ViewBounds.Position.Y + GetHeaderExtent() + (RowIndex * RowHeight) - ScrollOffset);
    return FRectangle(Position, ViewBounds.Width, RowHeight);
}

FRectangle FTreeView::ComputeDisclosureBounds(const FRectangle& RowBounds, int32 Depth) const
{
    const int32      Extent = GetArrowExtent();
    const IntVector2 Position(RowBounds.Position.X + (Depth * IndentPerLevel), RowBounds.Position.Y + ((RowHeight - Extent) / 2));

    return FRectangle(Position, Extent, Extent);
}

int32 FTreeView::ComputeRowExtent(const TSharedPtr<FTreeItem>& Item) const
{
    int32 Extent = (Item->GetDepth() * IndentPerLevel) + GetArrowExtent() + TREE_DISCLOSURE_SPACING;

    if (bReserveIconColumn)
    {
        Extent += TREE_ICON_SIZE + TREE_ICON_SPACING;
    }

    if (Font && !Item->Label.IsEmpty())
    {
        Extent += Font->MeasureWidth(StringView(Item->Label.Data(), Item->Label.Length()));
    }

    return Extent + TypeColumnWidth;
}

int32 FTreeView::ComputeLabelStartX(const FRectangle& RowBounds, const TSharedPtr<FTreeItem>& Item) const
{
    int32 LabelStartX = ComputeDisclosureBounds(RowBounds, Item->GetDepth()).GetRight() + TREE_DISCLOSURE_SPACING;

    if (bReserveIconColumn)
    {
        LabelStartX += TREE_ICON_SIZE + TREE_ICON_SPACING;
    }

    return LabelStartX;
}

bool FTreeView::IsAncestorOfSelection(const TSharedPtr<FTreeItem>& Item) const
{
    for (const TSharedPtr<FTreeItem>& Selected : Selection)
    {
        TWeakPtr<FTreeItem> Current = Selected ? Selected->Parent : TWeakPtr<FTreeItem>();
        while (Current.IsValid())
        {
            if (Current.Get() == Item.Get())
            {
                return true;
            }

            Current = Current->Parent;
        }
    }

    return false;
}

int32 FTreeView::GetArrowExtent() const
{
    const bool bHasBrush = ExpandedArrow.IsValid() || CollapsedArrow.IsValid();
    return bHasBrush ? ArrowSize : TREE_DISCLOSURE_SIZE;
}

int32 FTreeView::GetHeaderExtent() const
{
    return (LabelColumnHeader.IsEmpty() && TypeColumnHeader.IsEmpty()) ? 0 : HeaderHeight;
}

int32 FTreeView::FindRowAt(const IntVector2& ClientPosition) const
{
    const FRectangle& Bounds = GetContentRectangle();
    if (!Bounds.EncapsulatesPoint(ClientPosition))
    {
        return InvalidRowIndex;
    }

    const int32 LocalY = ClientPosition.Y - Bounds.Position.Y - GetHeaderExtent();
    if (LocalY < 0)
    {
        return InvalidRowIndex;
    }

    const int32 RowIndex = (LocalY + ScrollOffset) / RowHeight;
    return RowIndex < GetVisibleRows().Size() ? RowIndex : InvalidRowIndex;
}

int32 FTreeView::FindRowIndex(const TSharedPtr<FTreeItem>& Item) const
{
    return GetVisibleRows().Find(Item);
}

int32 FTreeView::GetCurrentRowIndex() const
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = GetVisibleRows();
    if (AnchorRowIndex >= 0 && AnchorRowIndex < Rows.Size())
    {
        return AnchorRowIndex;
    }

    for (int32 RowIndex = 0; RowIndex < Rows.Size(); ++RowIndex)
    {
        if (IsSelected(Rows[RowIndex]))
        {
            return RowIndex;
        }
    }

    return InvalidRowIndex;
}

int32 FTreeView::GetMaxScrollOffset() const
{
    return Math::Max(0, (GetVisibleRows().Size() * RowHeight) - (ViewHeight - GetHeaderExtent()));
}

void FTreeView::OnScrollBarMoved(int32 NewOffset)
{
    ScrollOffset = Math::Clamp(NewOffset, 0, GetMaxScrollOffset());
}

void FTreeView::ApplySelectionFromClick(int32 RowIndex, const FModifierKeyState& Modifiers)
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = GetVisibleRows();

    if (bAllowMultiSelect && Modifiers.IsShortcutChordDown())
    {
        const TSharedPtr<FTreeItem>& Item = Rows[RowIndex];
        if (Selection.Contains(Item))
        {
            Selection.Remove(Item);
        }
        else
        {
            Selection.Add(Item);
        }

        AnchorRowIndex = RowIndex;
        OnSelectionChangedDelegate.ExecuteIfBound(Selection);
        return;
    }

    if (bAllowMultiSelect && Modifiers.IsShiftDown() && AnchorRowIndex >= 0 && AnchorRowIndex < Rows.Size())
    {
        const int32 FirstRow = Math::Min(AnchorRowIndex, RowIndex);
        const int32 LastRow  = Math::Max(AnchorRowIndex, RowIndex);

        Selection.Clear();
        for (int32 Index = FirstRow; Index <= LastRow; ++Index)
        {
            Selection.Add(Rows[Index]);
        }

        OnSelectionChangedDelegate.ExecuteIfBound(Selection);
        return;
    }

    Selection.Clear();
    Selection.Add(Rows[RowIndex]);

    AnchorRowIndex = RowIndex;

    OnSelectionChangedDelegate.ExecuteIfBound(Selection);
}

void FTreeView::SelectSingleRow(int32 RowIndex)
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = GetVisibleRows();
    if (RowIndex < 0 || RowIndex >= Rows.Size())
    {
        return;
    }

    Selection.Clear();
    Selection.Add(Rows[RowIndex]);
    AnchorRowIndex = RowIndex;

    ScrollRowIntoView(RowIndex);
    OnSelectionChangedDelegate.ExecuteIfBound(Selection);
}

void FTreeView::MoveSelection(int32 Delta)
{
    const TArray<TSharedPtr<FTreeItem>>& Rows = GetVisibleRows();
    if (Rows.IsEmpty())
    {
        return;
    }

    const int32 CurrentRow = GetCurrentRowIndex();
    SelectSingleRow(CurrentRow == InvalidRowIndex ? 0 : Math::Clamp(CurrentRow + Delta, 0, Rows.LastIndex()));
}

void FTreeView::CollapseOrMoveToParent()
{
    const int32 RowIndex = GetCurrentRowIndex();
    if (RowIndex == InvalidRowIndex)
    {
        return;
    }

    const TSharedPtr<FTreeItem> Item = GetVisibleRows()[RowIndex];
    if (Item->HasChildren() && Item->bIsExpanded)
    {
        SetItemExpanded(Item, false);
        return;
    }

    if (Item->Parent.IsValid())
    {
        SelectSingleRow(FindRowIndex(Item->Parent.ToSharedPtr()));
    }
}

void FTreeView::ExpandOrMoveToFirstChild()
{
    const int32 RowIndex = GetCurrentRowIndex();
    if (RowIndex == InvalidRowIndex)
    {
        return;
    }

    const TSharedPtr<FTreeItem> Item = GetVisibleRows()[RowIndex];
    if (!Item->HasChildren())
    {
        return;
    }

    if (!Item->bIsExpanded)
    {
        SetItemExpanded(Item, true);
        return;
    }

    SelectSingleRow(FindRowIndex(Item->Children[0]));
}

void FTreeView::ScrollRowIntoView(int32 RowIndex)
{
    const int32 RowTop     = RowIndex * RowHeight;
    const int32 RowBottom  = RowTop + RowHeight;
    const int32 BandHeight = ViewHeight - GetHeaderExtent();

    if (RowTop < ScrollOffset)
    {
        ScrollOffset = RowTop;
    }
    else if (RowBottom > (ScrollOffset + BandHeight))
    {
        ScrollOffset = RowBottom - BandHeight;
    }

    ScrollOffset = Math::Clamp(ScrollOffset, 0, GetMaxScrollOffset());

    if (ScrollBar)
    {
        ScrollBar->SetOffset(ScrollOffset);
    }
}
