#include "Application/Elements/PropertyTable.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/ToolTipService.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32 PROPERTY_TABLE_CELL_INSET        = 4;
constexpr int32 PROPERTY_TABLE_ACCESSORY_SPACING = 12;
constexpr int32 PROPERTY_TABLE_REVERT_GLYPH_SIZE = 20;

constexpr float PROPERTY_TABLE_REVERT_ARC_THICKNESS = 1.5f;
constexpr float PROPERTY_TABLE_REVERT_HEAD_LENGTH   = 0.34f;
constexpr float PROPERTY_TABLE_REVERT_HEAD_WIDTH    = 0.26f;
constexpr float PROPERTY_TABLE_REVERT_ARC_START     = Math::Constants::PI * 1.05f;
constexpr float PROPERTY_TABLE_REVERT_ARC_END       = Math::Constants::PI * 2.0f;

static void DrawRevertArrow(FDrawCommandList& OutCommandList, int32 LayerId, const FRectangle& Bounds, const FUIBrush& Icon, const FFloatColor& Tint)
{
    if (Icon.IsValid())
    {
        OutCommandList.AddImage(LayerId, Bounds, Icon, Tint);
        return;
    }

    const float Extent = static_cast<float>(Math::Min(Bounds.Width, Bounds.Height));
    if (Extent <= 0.0f)
    {
        return;
    }

    const IntVector2 Middle = Bounds.GetCenter();
    const Vector2    Center = Vector2(static_cast<float>(Middle.X), static_cast<float>(Middle.Y) + (Extent * 0.15f));
    const float      Radius = Extent * 0.3f;

    OutCommandList.AddArc(LayerId, Center, Radius, PROPERTY_TABLE_REVERT_ARC_START, PROPERTY_TABLE_REVERT_ARC_END, Tint, PROPERTY_TABLE_REVERT_ARC_THICKNESS);

    const Vector2 Tail = Center + Vector2(Math::Cos(PROPERTY_TABLE_REVERT_ARC_START), Math::Sin(PROPERTY_TABLE_REVERT_ARC_START)) * Radius;
    const Vector2 Head = Tail + Vector2(0.0f, Extent * PROPERTY_TABLE_REVERT_HEAD_LENGTH);
    const float   Half = Extent * PROPERTY_TABLE_REVERT_HEAD_WIDTH * 0.5f;

    const Vector2 Corners[3] =
    {
        Vector2(Tail.X - Half, Tail.Y),
        Vector2(Tail.X + Half, Tail.Y),
        Vector2(Head.X, Head.Y),
    };

    OutCommandList.AddConvexPolygon(LayerId, TArrayView<const Vector2>(Corners, 3), Tint);
}

TSharedPtr<FPropertyTable> FPropertyTable::Create(const FDesc& Desc)
{
    TSharedPtr<FPropertyTable> NewPropertyTable = MakeSharedPtr<FPropertyTable>();
    NewPropertyTable->Initialize(Desc);
    return NewPropertyTable;
}

FPropertyTable::FPropertyTable()
    : FVisualElement()
    , Rows()
    , Font(nullptr)
    , Style()
    , RevertIcon()
    , DragOrigin()
    , LabelColumnFraction(0.4f)
    , DragStartFraction(0.4f)
    , LabelColumnWidth(0)
    , DragStartWidth(0)
    , EditorColumnInset(16)
    , RowHeight(FUIStyle::GetDefault().Metrics.RowHeight)
    , IndentPerLevel(12)
    , HoveredRowIndex(InvalidRowIndex)
    , HoveredRevertRow(InvalidRowIndex)
    , PressedRevertRow(InvalidRowIndex)
    , bShowRevertColumn(false)
    , bAlternateRowColors(false)
    , bIsDraggingDivider(false)
    , bIsDividerHovered(false)
{
}

FPropertyTable::~FPropertyTable() = default;

void FPropertyTable::Initialize(const FDesc& Desc)
{
    Font                = Desc.Font;
    Style               = Desc.Style;
    RevertIcon          = Desc.RevertIcon;
    LabelColumnWidth    = Math::Max(0, Desc.LabelColumnWidth);
    EditorColumnInset   = Math::Max(0, Desc.EditorColumnInset);
    RowHeight           = Math::Max(1, Desc.RowHeight);
    IndentPerLevel      = Math::Max(0, Desc.IndentPerLevel);
    bShowRevertColumn   = Desc.bShowRevertColumn;
    bAlternateRowColors = Desc.bAlternateRowColors;

    SetLabelColumnFraction(Desc.LabelColumnFraction);
}

IntVector2 FPropertyTable::ComputeDesiredSize() const
{
    return IntVector2(0, GetTotalRowHeight());
}

void FPropertyTable::OnArrange(const FRectangle& AllottedBounds)
{
    const int32 DividerX    = AllottedBounds.Position.X + GetDividerOffset(AllottedBounds);
    const int32 EditorRight = GetEditorColumnRight(AllottedBounds);

    for (int32 Index = 0; Index < Rows.Size(); ++Index)
    {
        const FPropertyRow& Row = Rows[Index];

        if (Row.LabelAccessory)
        {
            Row.LabelAccessory->Tick(GetLabelAccessoryRectangle(Index, AllottedBounds));
        }

        if (!Row.Editor)
        {
            continue;
        }

        FRectangle EditorBounds = GetRowRectangle(Index, AllottedBounds);
        if (!Row.bIsHeader)
        {
            EditorBounds.Position.X = DividerX + EditorColumnInset;
            EditorBounds.Width      = Math::Max(0, EditorRight - PROPERTY_TABLE_CELL_INSET - EditorBounds.Position.X);
        }

        EditorBounds = EditorBounds.Deflate(FMargin(0, Style.CellPadding.Top, 0, Style.CellPadding.Bottom));

        Row.Editor->Tick(EditorBounds);
    }
}

void FPropertyTable::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    for (const FPropertyRow& Row : Rows)
    {
        if (Row.Editor)
        {
            OutChildren.Add(Row.Editor);
        }

        if (Row.LabelAccessory)
        {
            OutChildren.Add(Row.LabelAccessory);
        }
    }
}

void FPropertyTable::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (IsPointOnDivider(ClientPosition))
    {
        return;
    }

    for (const FPropertyRow& Row : Rows)
    {
        if (Row.Editor)
        {
            Row.Editor->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }

        if (Row.LabelAccessory)
        {
            Row.LabelAccessory->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }
    }
}

int32 FPropertyTable::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& DefaultStyle = FUIStyle::GetDefault();
    const int32     DividerX     = AllottedGeometry.Bounds.Position.X + GetDividerOffset(AllottedGeometry.Bounds);
    const int32     LineWidth    = Math::Max(1, DefaultStyle.Metrics.SeparatorThickness);

    for (int32 Index = 0; Index < Rows.Size(); ++Index)
    {
        const FPropertyRow& Row       = Rows[Index];
        const FRectangle    RowBounds = GetRowRectangle(Index, AllottedGeometry.Bounds);

        if (Row.bIsHeader)
        {
            OutCommandList.AddBox(LayerId, RowBounds, DefaultStyle.Header.Fill);
        }
        else if (Index == HoveredRowIndex)
        {
            OutCommandList.AddBox(LayerId, RowBounds, Style.HoveredRowFill);
        }
        else
        {
            const bool bIsAlternate = bAlternateRowColors && (Index % 2) == 1;
            OutCommandList.AddBox(LayerId, RowBounds, bIsAlternate ? Style.AlternateRowFill : Style.RowFill);
        }

        const FRectangle RuleBounds(IntVector2(RowBounds.Position.X, RowBounds.GetBottom() - LineWidth), RowBounds.Width, LineWidth);
        OutCommandList.AddBox(LayerId, RuleBounds, Style.GridLine);

        if (Font && !Row.Label.IsEmpty())
        {
            const FRectangle LabelBounds = GetLabelRectangle(Index, AllottedGeometry.Bounds);
            const String     LabelText   = Font->ElideText(StringView(Row.Label.Data(), Row.Label.Length()), LabelBounds.Width);

            OutCommandList.AddText(LayerId, LabelBounds, LabelText, Font.Get(), DefaultStyle.Colors.Text);
        }

        if (IsRowModified(Index))
        {
            FFloatColor GlyphTint = Style.RevertGlyph;
            if (Index == PressedRevertRow)
            {
                GlyphTint = Style.RevertGlyphPressed;
            }
            else if (Index == HoveredRevertRow)
            {
                GlyphTint = Style.RevertGlyphHovered;
            }

            const FRectangle RevertBounds = GetRevertRectangle(Index, AllottedGeometry.Bounds);
            DrawRevertArrow(OutCommandList, LayerId, RevertBounds, RevertIcon, GlyphTint);
        }
    }

    if (!Rows.IsEmpty())
    {
        const int32 RowsHeight = Math::Min(AllottedGeometry.Bounds.Height, GetTotalRowHeight());

        FRectangle DividerLine = AllottedGeometry.Bounds;
        DividerLine.Position.X = DividerX;
        DividerLine.Width      = LineWidth;
        DividerLine.Height     = RowsHeight;

        const bool bIsDividerActive = bIsDraggingDivider || bIsDividerHovered;
        OutCommandList.AddBox(LayerId, DividerLine, bIsDividerActive ? DefaultStyle.Colors.Accent : Style.GridLine);

        if (bShowRevertColumn)
        {
            FRectangle RevertLine = DividerLine;
            RevertLine.Position.X = GetEditorColumnRight(AllottedGeometry.Bounds);

            OutCommandList.AddBox(LayerId, RevertLine, Style.GridLine);
        }
    }

    int32 NextLayerId = LayerId;
    for (const FPropertyRow& Row : Rows)
    {
        if (Row.Editor && Row.Editor->IsVisible())
        {
            const FDrawGeometry EditorGeometry(Row.Editor->GetContentRectangle(), AllottedGeometry.Scale);
            NextLayerId = Math::Max(NextLayerId, Row.Editor->OnDraw(EditorGeometry, OutCommandList, LayerId + 1));
        }

        if (Row.LabelAccessory && Row.LabelAccessory->IsVisible())
        {
            const FDrawGeometry AccessoryGeometry(Row.LabelAccessory->GetContentRectangle(), AllottedGeometry.Scale);
            NextLayerId = Math::Max(NextLayerId, Row.LabelAccessory->OnDraw(AccessoryGeometry, OutCommandList, LayerId + 1));
        }
    }

    return NextLayerId;
}

FEventResponse FPropertyTable::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const IntVector2 Position = CursorEvent.GetClientPosition();

    const int32 RevertRow = FindRevertRowAtPoint(Position);
    if (RevertRow != InvalidRowIndex)
    {
        PressedRevertRow = RevertRow;
        return FEventResponse::Handled();
    }

    if (!IsPointOnDivider(Position))
    {
        return FEventResponse::Unhandled();
    }

    bIsDraggingDivider = true;
    bIsDividerHovered  = true;
    DragOrigin         = CursorEvent.GetClientPosition();
    DragStartFraction  = LabelColumnFraction;
    DragStartWidth     = LabelColumnWidth;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FPropertyTable::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    if (PressedRevertRow != InvalidRowIndex)
    {
        const int32 ReleasedRow = PressedRevertRow;
        PressedRevertRow        = InvalidRowIndex;

        if (FindRevertRowAtPoint(CursorEvent.GetClientPosition()) == ReleasedRow)
        {
            Rows[ReleasedRow].OnRevert.ExecuteIfBound();
        }

        return FEventResponse::Handled();
    }

    if (!bIsDraggingDivider)
    {
        return FEventResponse::Unhandled();
    }

    bIsDraggingDivider = false;
    bIsDividerHovered  = IsPointOnDivider(CursorEvent.GetClientPosition());

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FPropertyTable::OnMouseMove(const FCursorEvent& CursorEvent)
{
    const IntVector2 Position = CursorEvent.GetClientPosition();

    if (!bIsDraggingDivider)
    {
        UpdateHoveredRow(Position);

        HoveredRevertRow  = FindRevertRowAtPoint(Position);
        bIsDividerHovered = IsPointOnDivider(Position);

        return bIsDividerHovered ? FEventResponse::Handled() : FEventResponse::Unhandled();
    }

    const FRectangle& Bounds = GetContentRectangle();
    if (Bounds.Width <= 0)
    {
        return FEventResponse::Handled();
    }

    if (LabelColumnWidth > 0)
    {
        const int32 Travel  = Position.X - DragOrigin.X;
        const int32 Lowest  = Math::RoundToInt(static_cast<float>(Bounds.Width) * MinLabelColumnFraction);
        const int32 Highest = Math::RoundToInt(static_cast<float>(Bounds.Width) * MaxLabelColumnFraction);

        LabelColumnWidth = Math::Clamp(DragStartWidth + Travel, Math::Max(1, Lowest), Math::Max(1, Highest));
    }
    else
    {
        const float Travel = static_cast<float>(Position.X - DragOrigin.X) / static_cast<float>(Bounds.Width);
        SetLabelColumnFraction(DragStartFraction + Travel);
    }

    OnArrange(Bounds);

    return FEventResponse::Handled();
}

FEventResponse FPropertyTable::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    ClearHoveredRow();

    HoveredRevertRow = InvalidRowIndex;
    PressedRevertRow = InvalidRowIndex;

    return FVisualElement::OnMouseLeft(CursorEvent);
}

bool FPropertyTable::GetCursor(ECursor& OutCursor) const
{
    if (bIsDraggingDivider || bIsDividerHovered)
    {
        OutCursor = ECursor::ResizeEW;
        return true;
    }

    if (HoveredRevertRow != InvalidRowIndex)
    {
        OutCursor = ECursor::Hand;
        return true;
    }

    return false;
}

FPropertyRow& FPropertyTable::AddRow(const String& Label, const TSharedPtr<FVisualElement>& Editor)
{
    FPropertyRow& NewRow = Rows.Emplace();
    NewRow.Label         = Label;
    NewRow.Editor        = Editor;

    if (Editor)
    {
        Editor->SetParentElement(AsWeakPtr());
    }

    return NewRow;
}

FPropertyRow& FPropertyTable::AddHeaderRow(const String& Label)
{
    FPropertyRow& NewRow = Rows.Emplace();
    NewRow.Label         = Label;
    NewRow.bIsHeader     = true;
    return NewRow;
}

void FPropertyTable::SetRowLabelAccessory(int32 Index, const TSharedPtr<FVisualElement>& Accessory)
{
    CHECK(Index >= 0 && Index < Rows.Size());

    Rows[Index].LabelAccessory = Accessory;

    if (Accessory)
    {
        Accessory->SetParentElement(AsWeakPtr());
    }
}

void FPropertyTable::ClearRows()
{
    ClearHoveredRow();

    Rows.Clear();

    bIsDraggingDivider = false;
    bIsDividerHovered  = false;
}

const FPropertyRow& FPropertyTable::GetRow(int32 Index) const
{
    CHECK(Index >= 0 && Index < Rows.Size());
    return Rows[Index];
}

void FPropertyTable::SetLabelColumnFraction(float InFraction)
{
    LabelColumnFraction = Math::Clamp(InFraction, MinLabelColumnFraction, MaxLabelColumnFraction);
}

FRectangle FPropertyTable::GetRowRectangle(int32 Index, const FRectangle& Bounds) const
{
    if (Index < 0 || Index >= Rows.Size())
    {
        return FRectangle();
    }

    return FRectangle(IntVector2(Bounds.Position.X, Bounds.Position.Y + GetRowOffset(Index)), Bounds.Width, GetRowHeight(Index));
}

FRectangle FPropertyTable::GetDividerRectangle(const FRectangle& Bounds) const
{
    if (Rows.IsEmpty() || Bounds.Width <= 0)
    {
        return FRectangle();
    }

    const int32 DividerX = Bounds.Position.X + GetDividerOffset(Bounds);
    const int32 Height   = Math::Min(Bounds.Height, GetTotalRowHeight());

    return FRectangle(IntVector2(DividerX - (DividerGrabWidth / 2), Bounds.Position.Y), DividerGrabWidth, Height);
}

int32 FPropertyTable::FindRowAtPoint(const IntVector2& ClientPosition) const
{
    const FRectangle& Bounds = GetContentRectangle();
    if (Rows.IsEmpty() || !Bounds.EncapsulatesPoint(ClientPosition))
    {
        return InvalidRowIndex;
    }

    const int32 Offset = ClientPosition.Y - Bounds.Position.Y;

    int32 Top = 0;
    for (int32 Index = 0; Index < Rows.Size(); ++Index)
    {
        const int32 Bottom = Top + GetRowHeight(Index);
        if (Offset < Bottom)
        {
            return Index;
        }

        Top = Bottom;
    }

    return InvalidRowIndex;
}

int32 FPropertyTable::GetRowHeight(int32 Index) const
{
    if (Index < 0 || Index >= Rows.Size())
    {
        return 0;
    }

    const int32 Override = Rows[Index].HeightOverride;
    return Override > 0 ? Override : RowHeight;
}

int32 FPropertyTable::GetRowOffset(int32 Index) const
{
    int32 Offset = 0;
    for (int32 Row = 0; Row < Index && Row < Rows.Size(); ++Row)
    {
        Offset += GetRowHeight(Row);
    }

    return Offset;
}

int32 FPropertyTable::GetTotalRowHeight() const
{
    return GetRowOffset(Rows.Size());
}

int32 FPropertyTable::GetEditorColumnRight(const FRectangle& Bounds) const
{
    const int32 Right = Bounds.GetRight();
    if (!bShowRevertColumn)
    {
        return Right;
    }

    return Math::Max(Bounds.Position.X, Right - Style.RevertColumnWidth);
}

FRectangle FPropertyTable::GetLabelRectangle(int32 Index, const FRectangle& Bounds) const
{
    if (Index < 0 || Index >= Rows.Size())
    {
        return FRectangle();
    }

    const FPropertyRow& Row        = Rows[Index];
    const FRectangle    RowBounds  = GetRowRectangle(Index, Bounds);
    const int32         LabelRight = Row.bIsHeader ? RowBounds.GetRight() : (Bounds.Position.X + GetDividerOffset(Bounds));

    FRectangle LabelBounds;
    LabelBounds.Position.X = RowBounds.Position.X + Style.LabelIndent + (Row.IndentLevel * IndentPerLevel);
    LabelBounds.Position.Y = RowBounds.Position.Y + (Font ? Font->GetTextBandOffset(RowBounds.Height) : 0);
    LabelBounds.Width      = Math::Max(0, LabelRight - Style.CellPadding.Right - LabelBounds.Position.X);
    LabelBounds.Height     = Font ? Font->GetTextBandHeight() : RowBounds.Height;

    return LabelBounds;
}

FRectangle FPropertyTable::GetLabelAccessoryRectangle(int32 Index, const FRectangle& Bounds) const
{
    if (Index < 0 || Index >= Rows.Size() || !Rows[Index].LabelAccessory)
    {
        return FRectangle();
    }

    const FRectangle RowBounds   = GetRowRectangle(Index, Bounds);
    const FRectangle LabelBounds = GetLabelRectangle(Index, Bounds);
    const IntVector2 Desired     = Rows[Index].LabelAccessory->ComputeDesiredSize();

    const int32 TextWidth = Font ? Font->MeasureWidth(StringView(Rows[Index].Label.Data(), Rows[Index].Label.Length())) : 0;
    const int32 Left      = LabelBounds.Position.X + TextWidth + PROPERTY_TABLE_ACCESSORY_SPACING;

    return FRectangle(IntVector2(Left, RowBounds.Position.Y + ((RowBounds.Height - Desired.Y) / 2)), Desired.X, Desired.Y);
}

FRectangle FPropertyTable::GetRevertRectangle(int32 Index, const FRectangle& Bounds) const
{
    if (!bShowRevertColumn || Index < 0 || Index >= Rows.Size())
    {
        return FRectangle();
    }

    const FRectangle RowBounds = GetRowRectangle(Index, Bounds);
    const int32      Extent    = Math::Min(PROPERTY_TABLE_REVERT_GLYPH_SIZE, Math::Min(Style.RevertColumnWidth, RowBounds.Height));
    const int32      ColumnX   = GetEditorColumnRight(Bounds);
    const IntVector2 Position  = IntVector2(ColumnX + ((Style.RevertColumnWidth - Extent) / 2), RowBounds.Position.Y + ((RowBounds.Height - Extent) / 2));

    return FRectangle(Position, Extent, Extent);
}

bool FPropertyTable::IsRowModified(int32 Index) const
{
    if (!bShowRevertColumn || Index < 0 || Index >= Rows.Size())
    {
        return false;
    }

    const FPropertyRow& Row = Rows[Index];
    return !Row.bIsHeader && Row.IsModified.IsBound() && Row.IsModified.Execute();
}

int32 FPropertyTable::FindRevertRowAtPoint(const IntVector2& ClientPosition) const
{
    const int32 RowIndex = FindRowAtPoint(ClientPosition);
    if (RowIndex == InvalidRowIndex || !IsRowModified(RowIndex))
    {
        return InvalidRowIndex;
    }

    return GetRevertRectangle(RowIndex, GetContentRectangle()).EncapsulatesPoint(ClientPosition) ? RowIndex : InvalidRowIndex;
}

const String& FPropertyTable::GetElidedLabel(int32 Index) const
{
    static const String EmptyLabel;

    const String& Label = Rows[Index].Label;
    if (!Font || Label.IsEmpty())
    {
        return EmptyLabel;
    }

    const int32 LabelWidth = GetLabelRectangle(Index, GetContentRectangle()).Width;
    return Font->MeasureWidth(StringView(Label.Data(), Label.Length())) > LabelWidth ? Label : EmptyLabel;
}

void FPropertyTable::UpdateHoveredRow(const IntVector2& ClientPosition)
{
    const int32 RowIndex = FindRowAtPoint(ClientPosition);
    if (RowIndex == HoveredRowIndex)
    {
        return;
    }

    ClearHoveredRow();
    HoveredRowIndex = RowIndex;

    if (RowIndex == InvalidRowIndex)
    {
        return;
    }

    const String& ToolTipText = Rows[RowIndex].ToolTipText.IsEmpty()
        ? GetElidedLabel(RowIndex)
        : Rows[RowIndex].ToolTipText;

    if (ToolTipText.IsEmpty())
    {
        return;
    }

    FToolTipService::Get().RequestTextToolTip(AsSharedPtr(), ToolTipText, Font);
}

void FPropertyTable::ClearHoveredRow()
{
    if (HoveredRowIndex == InvalidRowIndex)
    {
        return;
    }

    HoveredRowIndex = InvalidRowIndex;
    FToolTipService::Get().CancelToolTip(AsSharedPtr());
}

int32 FPropertyTable::GetDividerOffset(const FRectangle& Bounds) const
{
    const int32 Offset = LabelColumnWidth > 0
        ? LabelColumnWidth
        : Math::RoundToInt(static_cast<float>(Bounds.Width) * LabelColumnFraction);

    return Math::Clamp(Offset, 0, Bounds.Width);
}

bool FPropertyTable::IsPointOnDivider(const IntVector2& ClientPosition) const
{
    const FRectangle DividerBand = GetDividerRectangle(GetContentRectangle());
    return !DividerBand.IsEmpty() && DividerBand.EncapsulatesPoint(ClientPosition);
}
