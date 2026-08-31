#include "Application/Elements/PropertyTable.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/ToolTipService.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

/** @brief The space kept between a column edge and what the column holds, in pixels. */
constexpr int32 PROPERTY_TABLE_CELL_INSET = 4;

/** @brief How much brighter an alternate row is than the panel behind it. */
constexpr float PROPERTY_TABLE_ALTERNATE_ROW_LIGHTEN = 0.025f;

static FFloatColor GetAlternateRowColor(const FFloatColor& PanelColor)
{
    return FFloatColor(
        PanelColor.R + PROPERTY_TABLE_ALTERNATE_ROW_LIGHTEN,
        PanelColor.G + PROPERTY_TABLE_ALTERNATE_ROW_LIGHTEN,
        PanelColor.B + PROPERTY_TABLE_ALTERNATE_ROW_LIGHTEN,
        PanelColor.A);
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
    , DragOrigin()
    , LabelColumnFraction(0.4f)
    , DragStartFraction(0.4f)
    , RowHeight(FUIStyle::GetDefault().Metrics.RowHeight)
    , IndentPerLevel(12)
    , HoveredRowIndex(InvalidRowIndex)
    , bAlternateRowColors(true)
    , bIsDraggingDivider(false)
    , bIsDividerHovered(false)
{
}

FPropertyTable::~FPropertyTable() = default;

void FPropertyTable::Initialize(const FDesc& Desc)
{
    Font                = Desc.Font;
    RowHeight           = Math::Max(1, Desc.RowHeight);
    IndentPerLevel      = Math::Max(0, Desc.IndentPerLevel);
    bAlternateRowColors = Desc.bAlternateRowColors;

    SetLabelColumnFraction(Desc.LabelColumnFraction);
}

IntVector2 FPropertyTable::ComputeDesiredSize() const
{
    return IntVector2(0, Rows.Size() * RowHeight);
}

void FPropertyTable::OnArrange(const FRectangle& AllottedBounds)
{
    const int32 DividerX = AllottedBounds.Position.X + GetDividerOffset(AllottedBounds);

    for (int32 Index = 0; Index < Rows.Size(); ++Index)
    {
        const FPropertyRow& Row = Rows[Index];
        if (!Row.Editor)
        {
            continue;
        }

        FRectangle EditorBounds = GetRowRectangle(Index, AllottedBounds);
        if (!Row.bIsHeader)
        {
            EditorBounds.Position.X = DividerX + PROPERTY_TABLE_CELL_INSET;
            EditorBounds.Width      = Math::Max(0, AllottedBounds.GetRight() - PROPERTY_TABLE_CELL_INSET - EditorBounds.Position.X);
        }

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
    }
}

int32 FPropertyTable::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&   Style             = FUIStyle::GetDefault();
    const FFloatColor AlternateRowColor = GetAlternateRowColor(Style.Colors.PanelBackground);
    const int32       DividerX          = AllottedGeometry.Bounds.Position.X + GetDividerOffset(AllottedGeometry.Bounds);

    for (int32 Index = 0; Index < Rows.Size(); ++Index)
    {
        const FPropertyRow& Row       = Rows[Index];
        const FRectangle    RowBounds = GetRowRectangle(Index, AllottedGeometry.Bounds);

        if (Row.bIsHeader)
        {
            OutCommandList.AddBox(LayerId, RowBounds, Style.Colors.ControlNormal);
        }
        else if (bAlternateRowColors && (Index % 2) == 1)
        {
            OutCommandList.AddBox(LayerId, RowBounds, AlternateRowColor);
        }

        if (Font && !Row.Label.IsEmpty())
        {
            const int32 LabelRight = Row.bIsHeader ? RowBounds.GetRight() : DividerX;

            FRectangle LabelBounds;
            LabelBounds.Position.X = RowBounds.Position.X + PROPERTY_TABLE_CELL_INSET + (Row.IndentLevel * IndentPerLevel);
            LabelBounds.Position.Y = RowBounds.Position.Y + Font->GetTextBandOffset(RowBounds.Height);
            LabelBounds.Width      = Math::Max(0, LabelRight - PROPERTY_TABLE_CELL_INSET - LabelBounds.Position.X);
            LabelBounds.Height     = Font->GetTextBandHeight();

            OutCommandList.AddText(LayerId, LabelBounds, Row.Label, Font.Get(), Style.Colors.Text);
        }
    }

    if (!Rows.IsEmpty())
    {
        FRectangle DividerLine = AllottedGeometry.Bounds;
        DividerLine.Position.X = DividerX;
        DividerLine.Width      = Style.Metrics.SeparatorThickness;
        DividerLine.Height     = Math::Min(DividerLine.Height, Rows.Size() * RowHeight);

        const bool bIsDividerActive = bIsDraggingDivider || bIsDividerHovered;
        OutCommandList.AddLine(LayerId, DividerLine, bIsDividerActive ? Style.Colors.Accent : Style.Colors.Border);
    }

    int32 NextLayerId = LayerId;
    for (const FPropertyRow& Row : Rows)
    {
        if (Row.Editor && Row.Editor->IsVisible())
        {
            const FDrawGeometry EditorGeometry(Row.Editor->GetContentRectangle(), AllottedGeometry.Scale);
            NextLayerId = Math::Max(NextLayerId, Row.Editor->OnDraw(EditorGeometry, OutCommandList, LayerId + 1));
        }
    }

    return NextLayerId;
}

FEventResponse FPropertyTable::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft || !IsPointOnDivider(CursorEvent.GetClientPosition()))
    {
        return FEventResponse::Unhandled();
    }

    bIsDraggingDivider = true;
    bIsDividerHovered  = true;
    DragOrigin         = CursorEvent.GetClientPosition();
    DragStartFraction  = LabelColumnFraction;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FPropertyTable::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (!bIsDraggingDivider || CursorEvent.GetKey() != Keys::MouseButtonLeft)
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

        bIsDividerHovered = IsPointOnDivider(Position);
        return bIsDividerHovered ? FEventResponse::Handled() : FEventResponse::Unhandled();
    }

    const FRectangle& Bounds = GetContentRectangle();
    if (Bounds.Width <= 0)
    {
        return FEventResponse::Handled();
    }

    const float Travel = static_cast<float>(Position.X - DragOrigin.X) / static_cast<float>(Bounds.Width);
    SetLabelColumnFraction(DragStartFraction + Travel);
    OnArrange(Bounds);

    return FEventResponse::Handled();
}

FEventResponse FPropertyTable::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    ClearHoveredRow();
    return FVisualElement::OnMouseLeft(CursorEvent);
}

bool FPropertyTable::GetCursor(ECursor& OutCursor) const
{
    if (!bIsDraggingDivider && !bIsDividerHovered)
    {
        return false;
    }

    OutCursor = ECursor::ResizeEW;
    return true;
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

    return FRectangle(IntVector2(Bounds.Position.X, Bounds.Position.Y + (Index * RowHeight)), Bounds.Width, RowHeight);
}

FRectangle FPropertyTable::GetDividerRectangle(const FRectangle& Bounds) const
{
    if (Rows.IsEmpty() || Bounds.Width <= 0)
    {
        return FRectangle();
    }

    const int32 DividerX = Bounds.Position.X + GetDividerOffset(Bounds);
    const int32 Height   = Math::Min(Bounds.Height, Rows.Size() * RowHeight);

    return FRectangle(IntVector2(DividerX - (DividerGrabWidth / 2), Bounds.Position.Y), DividerGrabWidth, Height);
}

int32 FPropertyTable::FindRowAtPoint(const IntVector2& ClientPosition) const
{
    const FRectangle& Bounds = GetContentRectangle();
    if (Rows.IsEmpty() || !Bounds.EncapsulatesPoint(ClientPosition))
    {
        return InvalidRowIndex;
    }

    const int32 Index = (ClientPosition.Y - Bounds.Position.Y) / RowHeight;
    return (Index >= 0 && Index < Rows.Size()) ? Index : InvalidRowIndex;
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

    if (RowIndex == InvalidRowIndex || Rows[RowIndex].ToolTipText.IsEmpty())
    {
        return;
    }

    FToolTipService::Get().RequestTextToolTip(AsSharedPtr(), Rows[RowIndex].ToolTipText, Font);
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
    return Math::Clamp(Math::RoundToInt(static_cast<float>(Bounds.Width) * LabelColumnFraction), 0, Bounds.Width);
}

bool FPropertyTable::IsPointOnDivider(const IntVector2& ClientPosition) const
{
    const FRectangle DividerBand = GetDividerRectangle(GetContentRectangle());
    return !DividerBand.IsEmpty() && DividerBand.EncapsulatesPoint(ClientPosition);
}
