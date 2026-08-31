#include "Application/Elements/TileView.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32       TILE_INNER_PADDING  = 4;
constexpr int32       TILE_ICON_LABEL_GAP = 4;
constexpr const CHAR* TILE_LABEL_ELLIPSIS = "..";

TSharedPtr<FTileView> FTileView::Create(const FDesc& Desc)
{
    TSharedPtr<FTileView> NewTileView = MakeSharedPtr<FTileView>();
    NewTileView->Initialize(Desc);
    return NewTileView;
}

FTileView::FTileView()
    : FVisualElement()
    , Items()
    , SelectedIndices()
    , Font(nullptr)
    , TileSize(80, 92)
    , TileSpacing(8)
    , IconSize(48)
    , ScrollOffset(0)
    , ContentHeight(0)
    , ViewHeight(0)
    , HoveredIndex(InvalidTileIndex)
    , AnchorIndex(InvalidTileIndex)
    , bAllowMultiSelect(true)
    , OnSelectionChangedDelegate()
    , OnItemActivatedDelegate()
{
}

FTileView::~FTileView() = default;

void FTileView::Initialize(const FDesc& Desc)
{
    Font                       = Desc.Font;
    TileSize                   = IntVector2(Math::Max(Desc.TileSize.X, 1), Math::Max(Desc.TileSize.Y, 1));
    TileSpacing                = Math::Max(Desc.TileSpacing, 0);
    IconSize                   = Math::Max(Desc.IconSize, 0);
    bAllowMultiSelect          = Desc.bAllowMultiSelect;
    OnSelectionChangedDelegate = Desc.OnSelectionChanged;
    OnItemActivatedDelegate    = Desc.OnItemActivated;
}

IntVector2 FTileView::ComputeDesiredSize() const
{
    return TileSize;
}

void FTileView::OnArrange(const FRectangle& AllottedBounds)
{
    ViewHeight    = AllottedBounds.Height;
    ContentHeight = ComputeContentHeight(AllottedBounds.Width);
    ScrollOffset  = Math::Clamp(ScrollOffset, 0, GetMaxScrollOffset());
}

int32 FTileView::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&    Style  = FUIStyle::GetDefault();
    const FRectangle   Bounds = AllottedGeometry.Bounds;
    const FCornerRadii Radii(Style.Metrics.CornerRadius);

    OutCommandList.PushClip(LayerId, Bounds);

    int32 MaxLayerId = LayerId;
    for (int32 Index = 0; Index < Items.Size(); ++Index)
    {
        const FRectangle Tile = ComputeTileBounds(Index, Bounds);
        if (Tile.Position.Y > Bounds.GetBottom())
        {
            break;
        }

        if (Tile.GetBottom() < Bounds.Position.Y)
        {
            continue;
        }

        const FTileItem& Item = Items[Index];

        if (IsSelected(Index))
        {
            OutCommandList.AddBox(LayerId, Tile, Style.Colors.TextSelectionBackground, Radii);
        }
        else if (Index == HoveredIndex)
        {
            OutCommandList.AddBox(LayerId, Tile, Style.Colors.ControlHovered, Radii);
        }

        FRectangle IconBounds;
        IconBounds.Width      = Math::Min(IconSize, Tile.Width);
        IconBounds.Height     = Math::Min(IconSize, Tile.Height);
        IconBounds.Position.X = Tile.Position.X + ((Tile.Width - IconBounds.Width) / 2);
        IconBounds.Position.Y = Tile.Position.Y + TILE_INNER_PADDING;

        if (Item.Icon.IsValid())
        {
            OutCommandList.AddImage(LayerId + 1, IconBounds, Item.Icon, FFloatColor::White);
            MaxLayerId = Math::Max(MaxLayerId, LayerId + 1);
        }

        const int32 LabelTop = IconBounds.GetBottom() + TILE_ICON_LABEL_GAP;
        if (Font && !Item.Label.IsEmpty() && LabelTop < Tile.GetBottom())
        {
            const String     LabelText = ElideLabel(Item.Label, Tile.Width - (TILE_INNER_PADDING * 2));
            const IntVector2 LabelSize(Font->MeasureWidth(StringView(LabelText.Data(), LabelText.Length())), Font->GetLineHeight());

            FRectangle LabelBounds = Tile;
            LabelBounds.Position.Y = LabelTop;
            LabelBounds.Height     = Tile.GetBottom() - LabelTop;
            LabelBounds            = FRectangle::AlignInBounds(LabelBounds, LabelSize, EHorizontalAlignment::Center, EVerticalAlignment::Top);

            OutCommandList.AddText(LayerId + 2, LabelBounds, LabelText, Font.Get(), Style.Colors.Text);
            MaxLayerId = Math::Max(MaxLayerId, LayerId + 2);
        }
    }

    OutCommandList.PopClip(MaxLayerId);
    return MaxLayerId;
}

FEventResponse FTileView::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const int32 Index = FindTileAt(CursorEvent.GetClientPosition());
    if (Index == InvalidTileIndex)
    {
        ClearSelection();
        return FEventResponse::Handled();
    }

    const FModifierKeyState& Modifiers = CursorEvent.GetModifierKeys();
    SelectTile(Index, bAllowMultiSelect && Modifiers.IsShortcutChordDown(), bAllowMultiSelect && Modifiers.IsShiftDown());

    OnSelectionChangedDelegate.ExecuteIfBound(SelectedIndices);
    return FEventResponse::Handled();
}

FEventResponse FTileView::OnMouseMove(const FCursorEvent& CursorEvent)
{
    HoveredIndex = FindTileAt(CursorEvent.GetClientPosition());
    return FEventResponse::Unhandled();
}

FEventResponse FTileView::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    HoveredIndex = InvalidTileIndex;
    return FEventResponse::Unhandled();
}

FEventResponse FTileView::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const int32 Index = FindTileAt(CursorEvent.GetClientPosition());
    if (Index == InvalidTileIndex)
    {
        return FEventResponse::Unhandled();
    }

    OnItemActivatedDelegate.ExecuteIfBound(Index);
    return FEventResponse::Handled();
}

FEventResponse FTileView::OnMouseScroll(const FCursorEvent& CursorEvent)
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

    const int32 Delta = static_cast<int32>(CursorEvent.GetScrollDelta() * static_cast<float>(DefaultScrollAmountPerWheelStep));
    ScrollOffset = Math::Clamp(ScrollOffset - Delta, 0, MaxScrollOffset);
    return FEventResponse::Handled();
}

bool FTileView::SupportsKeyboardFocus() const
{
    return true;
}

void FTileView::SetItems(const TArray<FTileItem>& InItems)
{
    Items = InItems;

    SelectedIndices.Clear();

    HoveredIndex = InvalidTileIndex;
    AnchorIndex  = InvalidTileIndex;
    ScrollOffset = 0;
}

void FTileView::ClearSelection()
{
    if (SelectedIndices.IsEmpty())
    {
        return;
    }

    SelectedIndices.Clear();

    AnchorIndex = InvalidTileIndex;

    OnSelectionChangedDelegate.ExecuteIfBound(SelectedIndices);
}

bool FTileView::IsSelected(int32 Index) const
{
    return SelectedIndices.Contains(Index);
}

void FTileView::SetTileSize(const IntVector2& InTileSize)
{
    TileSize = IntVector2(Math::Max(InTileSize.X, 1), Math::Max(InTileSize.Y, 1));
}

void FTileView::SetScrollOffset(int32 InScrollOffset)
{
    ScrollOffset = Math::Clamp(InScrollOffset, 0, GetMaxScrollOffset());
}

int32 FTileView::GetMaxScrollOffset() const
{
    return Math::Max(ContentHeight - ViewHeight, 0);
}

int32 FTileView::ResolveNumColumns(int32 AvailableWidth) const
{
    return Math::Max((AvailableWidth + TileSpacing) / (TileSize.X + TileSpacing), 1);
}

int32 FTileView::ComputeContentHeight(int32 AvailableWidth) const
{
    if (Items.IsEmpty())
    {
        return 0;
    }

    const int32 NumColumns = ResolveNumColumns(AvailableWidth);
    const int32 NumRows    = ((Items.Size() + NumColumns) - 1) / NumColumns;
    return (NumRows * (TileSize.Y + TileSpacing)) - TileSpacing;
}

FRectangle FTileView::ComputeTileBounds(int32 Index, const FRectangle& Bounds) const
{
    const int32 NumColumns = ResolveNumColumns(Bounds.Width);
    const int32 Column     = Index % NumColumns;
    const int32 Row        = Index / NumColumns;

    FRectangle Tile;
    Tile.Width      = TileSize.X;
    Tile.Height     = TileSize.Y;
    Tile.Position.X = Bounds.Position.X + (Column * (TileSize.X + TileSpacing));
    Tile.Position.Y = (Bounds.Position.Y + (Row * (TileSize.Y + TileSpacing))) - ScrollOffset;
    return Tile;
}

int32 FTileView::FindTileAt(const IntVector2& ClientPosition) const
{
    const FRectangle Bounds = GetContentRectangle();
    if (!Bounds.EncapsulatesPoint(ClientPosition))
    {
        return InvalidTileIndex;
    }

    for (int32 Index = 0; Index < Items.Size(); ++Index)
    {
        if (ComputeTileBounds(Index, Bounds).EncapsulatesPoint(ClientPosition))
        {
            return Index;
        }
    }

    return InvalidTileIndex;
}

String FTileView::ElideLabel(const String& InLabel, int32 MaxWidth) const
{
    if (!Font || MaxWidth <= 0)
    {
        return InLabel;
    }

    if (Font->MeasureWidth(StringView(InLabel.Data(), InLabel.Length())) <= MaxWidth)
    {
        return InLabel;
    }

    const int32 EllipsisWidth = Font->MeasureWidth(StringView(TILE_LABEL_ELLIPSIS));

    int32 NumCharacters = InLabel.Length();
    while (NumCharacters > 0 && (Font->MeasureWidth(StringView(InLabel.Data(), NumCharacters)) + EllipsisWidth) > MaxWidth)
    {
        --NumCharacters;
    }

    return InLabel.SubString(0, NumCharacters) + TILE_LABEL_ELLIPSIS;
}

void FTileView::SelectTile(int32 Index, bool bToggle, bool bExtend)
{
    if (bToggle)
    {
        if (SelectedIndices.Contains(Index))
        {
            SelectedIndices.Remove(Index);
        }
        else
        {
            SelectedIndices.Add(Index);
        }

        AnchorIndex = Index;
        return;
    }

    if (bExtend && AnchorIndex != InvalidTileIndex)
    {
        const int32 First = Math::Min(AnchorIndex, Index);
        const int32 Last  = Math::Max(AnchorIndex, Index);

        SelectedIndices.Clear();

        for (int32 Current = First; Current <= Last; ++Current)
        {
            SelectedIndices.Add(Current);
        }

        return;
    }

    SelectedIndices.Clear();
    SelectedIndices.Add(Index);

    AnchorIndex = Index;
}
