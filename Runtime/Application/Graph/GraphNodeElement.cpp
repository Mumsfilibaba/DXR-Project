#include "Application/Draw/DrawCommandList.h"
#include "Application/Graph/GraphNodeElement.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

// The space either side of a title and of a pin label, in graph space
constexpr int32 GRAPH_NODE_PADDING = 8;

// The gap kept between the widest input label and the widest output label, in graph space
constexpr int32 GRAPH_NODE_LABEL_GAP = 24;

// How much larger the circle of a hovered pin is drawn
constexpr float GRAPH_NODE_PIN_HOVER_SCALE = 1.5f;

// How far the rule between the stacked input and output groups is held off each edge, in graph space
constexpr int32 GRAPH_NODE_SEPARATOR_INSET = 6;

TSharedPtr<FGraphNodeElement> FGraphNodeElement::Create(const FGraphNode& Node, const TSharedPtr<IFontFace>& InFont)
{
    TSharedPtr<FGraphNodeElement> NewElement = MakeSharedPtr<FGraphNodeElement>();
    NewElement->Node = Node;
    NewElement->Font = InFont;

    if (Node.Content)
    {
        NewElement->SetContent(Node.Content);
    }

    return NewElement;
}

FGraphNodeElement::FGraphNodeElement()
    : FCompoundElement()
    , Node()
    , Style()
    , Font(nullptr)
    , PinCenters()
    , Zoom(1.0f)
    , HoveredPinId(-1)
    , bIsSelected(false)
{
}

FGraphNodeElement::~FGraphNodeElement() = default;

IntVector2 FGraphNodeElement::ComputeDesiredSize() const
{
    int32 TitleWidth      = 0;
    int32 InputLabelWidth = 0;
    int32 OutputLabelWidth = 0;

    if (Font)
    {
        TitleWidth = Font->MeasureWidth(StringView(Node.Title.Data(), Node.Title.Length()));

        for (const FGraphPin& Pin : Node.Pins)
        {
            const int32 LabelWidth = Font->MeasureWidth(StringView(Pin.Name.Data(), Pin.Name.Length()));
            if (Pin.Direction == EGraphPinDirection::Input)
            {
                InputLabelWidth = Math::Max(InputLabelWidth, LabelWidth);
            }
            else
            {
                OutputLabelWidth = Math::Max(OutputLabelWidth, LabelWidth);
            }
        }
    }

    const int32 PinsWidth = Style.bStackPinRows
        ? (Math::Max(InputLabelWidth, OutputLabelWidth) + (GRAPH_NODE_PADDING * 2) + (PinRadius * 4))
        : (InputLabelWidth + OutputLabelWidth + GRAPH_NODE_LABEL_GAP + (GRAPH_NODE_PADDING * 2) + (PinRadius * 4));

    IntVector2 DesiredSize;
    DesiredSize.X = Math::Max(MinimumWidth, Math::Max(TitleWidth + (GRAPH_NODE_PADDING * 2), PinsWidth));
    DesiredSize.Y = TitleHeight + (CountPinRows() * PinRowHeight) + GRAPH_NODE_PADDING;

    if (Content)
    {
        const IntVector2 ContentSize = Content->GetCachedDesiredSize();
        DesiredSize.X = Math::Max(DesiredSize.X, ContentSize.X + (GRAPH_NODE_PADDING * 2));
        DesiredSize.Y += ContentSize.Y + GRAPH_NODE_PADDING;
    }

    return DesiredSize;
}

void FGraphNodeElement::OnArrange(const FRectangle& AllottedBounds)
{
    PinCenters.Clear();
    PinCenters.Resize(Node.Pins.Size());

    const int32 RowHeight   = Scaled(PinRowHeight);
    const int32 PinInset    = Scaled(GRAPH_NODE_PADDING + PinRadius);
    const int32 FirstRowTop = AllottedBounds.Position.Y + Scaled(TitleHeight);
    const int32 NumInputs   = Style.bStackPinRows ? CountPins(EGraphPinDirection::Input) : 0;

    int32 InputRow  = 0;
    int32 OutputRow = NumInputs;

    for (int32 Index = 0; Index < Node.Pins.Size(); ++Index)
    {
        const FGraphPin& Pin      = Node.Pins[Index];
        const bool       bIsInput = Pin.Direction == EGraphPinDirection::Input;
        const int32      Row      = bIsInput ? InputRow++ : OutputRow++;
        const int32      CenterX  = bIsInput ? AllottedBounds.Position.X + PinInset : AllottedBounds.GetRight() - PinInset;
        const int32      CenterY  = FirstRowTop + (Row * RowHeight) + (RowHeight / 2);

        PinCenters[Index] = Vector2(static_cast<float>(CenterX), static_cast<float>(CenterY));
    }

    if (!Content)
    {
        return;
    }

    const int32 Inset         = Scaled(GRAPH_NODE_PADDING);
    const int32 ContentTop    = FirstRowTop + (CountPinRows() * RowHeight);
    const int32 ContentBottom = Math::Max(AllottedBounds.GetBottom() - Inset, ContentTop);

    const FRectangle ContentBounds(
        IntVector2(AllottedBounds.Position.X + Inset, ContentTop),
        Math::Max(AllottedBounds.Width - (Inset * 2), 0),
        ContentBottom - ContentTop);

    Content->Tick(ContentBounds);
}

int32 FGraphNodeElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&   UIStyle = FUIStyle::GetDefault();
    const FRectangle& Bounds  = AllottedGeometry.Bounds;

    const float        CornerRadius = Style.CornerRadius * Zoom;
    const FCornerRadii Corners(CornerRadius);

    const bool        bIsMuted  = Node.bIsMuted;
    const FFloatColor BodyColor = bIsMuted ? Style.MutedBody : Style.Body;
    const FFloatColor TextColor = bIsMuted ? Style.MutedText : Style.Text;

    FFloatColor TitleColor = Node.TitleTint;
    if (bIsMuted)
    {
        TitleColor.A *= Style.MutedTintOpacity;
    }

    OutCommandList.AddBox(LayerId, Bounds, BodyColor, Corners);
    OutCommandList.AddBox(LayerId, GetTitleBounds(Bounds), TitleColor, FCornerRadii::Top(CornerRadius));

    const FFloatColor BorderColor = bIsSelected
        ? UIStyle.Colors.Accent
        : (bIsMuted ? Style.MutedBorder : Style.Border);

    const float Thickness = bIsSelected ? UIStyle.Metrics.BorderThickness * 2.0f : UIStyle.Metrics.BorderThickness;
    OutCommandList.AddBoxOutline(LayerId + 1, Bounds, BorderColor, Thickness, Corners);

    DrawPinSeparator(Bounds, OutCommandList, LayerId + 1, BorderColor);

    if (Font)
    {
        const int32 Inset = Scaled(GRAPH_NODE_PADDING);
        OutCommandList.AddText(LayerId + 1, GetTitleBounds(Bounds).Deflate(FMargin(Inset, 0, Inset, 0)), Node.Title, Font.Get(), TextColor);

        for (int32 Index = 0; Index < Node.Pins.Size(); ++Index)
        {
            const FGraphPin& Pin      = Node.Pins[Index];
            const bool       bIsInput = Pin.Direction == EGraphPinDirection::Input;
            const Vector2&   Center   = PinCenters[Index];

            const int32 RowHeight = Scaled(PinRowHeight);
            const int32 LabelGap  = Scaled(PinRadius * 2);
            const int32 Top       = static_cast<int32>(Center.Y) - (RowHeight / 2);

            const int32 LabelWidth = bIsInput
                ? Math::Max(Bounds.Width - (Inset * 2), 0)
                : Font->MeasureWidth(StringView(Pin.Name.Data(), Pin.Name.Length()));

            const int32 LabelLeft = bIsInput
                ? (static_cast<int32>(Center.X) + LabelGap)
                : (static_cast<int32>(Center.X) - LabelGap - LabelWidth);

            OutCommandList.AddText(LayerId + 1, FRectangle(IntVector2(LabelLeft, Top), LabelWidth, RowHeight), Pin.Name, Font.Get(), TextColor);
        }
    }

    for (int32 Index = 0; Index < Node.Pins.Size(); ++Index)
    {
        const FGraphPin& Pin        = Node.Pins[Index];
        const bool       bIsHovered = Pin.PinId == HoveredPinId;
        const float      Radius     = static_cast<float>(Scaled(PinRadius)) * (bIsHovered ? GRAPH_NODE_PIN_HOVER_SCALE : 1.0f);

        FFloatColor PinColor = Pin.Tint;
        if (bIsMuted)
        {
            PinColor.A *= Style.MutedTintOpacity;
        }

        OutCommandList.AddCircleFilled(LayerId + 1, PinCenters[Index], Radius, PinColor);

        if (Style.PinOutline.A > 0.0f)
        {
            OutCommandList.AddCircle(LayerId + 1, PinCenters[Index], Radius, Style.PinOutline, UIStyle.Metrics.BorderThickness);
        }
    }

    return Content ? FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 2) : LayerId + 1;
}

void FGraphNodeElement::DrawPinSeparator(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId, const FFloatColor& Color) const
{
    if (!Style.bStackPinRows)
    {
        return;
    }

    const int32 NumInputs  = CountPins(EGraphPinDirection::Input);
    const int32 NumOutputs = CountPins(EGraphPinDirection::Output);

    if (NumInputs <= 0 || NumOutputs <= 0)
    {
        return;
    }

    const int32 Inset = Scaled(GRAPH_NODE_SEPARATOR_INSET);
    const int32 Y     = Bounds.Position.Y + Scaled(TitleHeight) + (NumInputs * Scaled(PinRowHeight));

    OutCommandList.AddLine(LayerId, FRectangle(IntVector2(Bounds.Position.X + Inset, Y), Math::Max(Bounds.Width - (Inset * 2), 0), 1), Color);
}

void FGraphNodeElement::SetZoom(float InZoom)
{
    Zoom = InZoom;
}

void FGraphNodeElement::SetStyle(const FGraphNodeStyle& InStyle)
{
    Style = InStyle;
}

void FGraphNodeElement::SetSelected(bool bInIsSelected)
{
    bIsSelected = bInIsSelected;
}

void FGraphNodeElement::SetHoveredPin(int32 InPinId)
{
    HoveredPinId = InPinId;
}

bool FGraphNodeElement::GetPinCenter(int32 PinId, Vector2& OutPosition) const
{
    for (int32 Index = 0; Index < Node.Pins.Size() && Index < PinCenters.Size(); ++Index)
    {
        if (Node.Pins[Index].PinId == PinId)
        {
            OutPosition = PinCenters[Index];
            return true;
        }
    }

    return false;
}

int32 FGraphNodeElement::FindPinAt(const IntVector2& ClientPosition) const
{
    const float   GrabRadius = Math::Max(static_cast<float>(Scaled(PinRadius)) * 2.0f, 6.0f);
    const Vector2 Position   = Vector2(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));

    for (int32 Index = 0; Index < Node.Pins.Size() && Index < PinCenters.Size(); ++Index)
    {
        if ((PinCenters[Index] - Position).GetLength() <= GrabRadius)
        {
            return Node.Pins[Index].PinId;
        }
    }

    return -1;
}

FRectangle FGraphNodeElement::GetTitleBounds(const FRectangle& Bounds) const
{
    return FRectangle(Bounds.Position, Bounds.Width, Math::Min(Scaled(TitleHeight), Bounds.Height));
}

int32 FGraphNodeElement::Scaled(int32 GraphSpaceLength) const
{
    return static_cast<int32>(static_cast<float>(GraphSpaceLength) * Zoom);
}

int32 FGraphNodeElement::CountPinRows() const
{
    const int32 NumInputs  = CountPins(EGraphPinDirection::Input);
    const int32 NumOutputs = CountPins(EGraphPinDirection::Output);

    return Style.bStackPinRows ? (NumInputs + NumOutputs) : Math::Max(NumInputs, NumOutputs);
}

int32 FGraphNodeElement::CountPins(EGraphPinDirection Direction) const
{
    int32 Count = 0;
    for (const FGraphPin& Pin : Node.Pins)
    {
        Count += (Pin.Direction == Direction) ? 1 : 0;
    }

    return Count;
}
