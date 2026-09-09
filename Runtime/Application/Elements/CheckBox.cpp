#include "Application/Elements/CheckBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FCheckBox> FCheckBox::Create(const FDesc& Desc)
{
    TSharedPtr<FCheckBox> NewCheckBox = MakeSharedPtr<FCheckBox>();
    NewCheckBox->Initialize(Desc);
    return NewCheckBox;
}

FCheckBox::FCheckBox()
    : FInteractiveElement()
    , LabelText(nullptr)
    , CheckState(ECheckBoxState::Unchecked)
    , BoxSize(static_cast<int32>(static_cast<float>(FUIStyle::GetDefault().Metrics.RowHeight) * 0.8f))
    , LabelSpacing(8)
    , bIsTriState(false)
    , OnStateChangedDelegate()
{
}

FCheckBox::~FCheckBox() = default;

void FCheckBox::Initialize(const FDesc& Desc)
{
    CheckState             = Desc.InitialState;
    BoxSize                = Math::Max(Desc.BoxSize, 1);
    LabelSpacing           = Math::Max(Desc.LabelSpacing, 0);
    bIsTriState            = Desc.bIsTriState;
    OnStateChangedDelegate = Desc.OnStateChanged;

    if (Desc.Label)
    {
        SetContent(Desc.Label);
        return;
    }

    if (!Desc.Text.IsEmpty())
    {
        FTextBlock::FDesc LabelDesc;
        LabelDesc.Text            = Desc.Text;
        LabelDesc.Font            = Desc.Font;
        LabelDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

        LabelText = FTextBlock::Create(LabelDesc);
        SetContent(LabelText);
    }
}

IntVector2 FCheckBox::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(BoxSize, BoxSize);

    if (Content)
    {
        const IntVector2 LabelSize = Content->GetCachedDesiredSize();
        DesiredSize.X += LabelSpacing + LabelSize.X;
        DesiredSize.Y = Math::Max(DesiredSize.Y, LabelSize.Y);
    }

    return DesiredSize;
}

void FCheckBox::OnArrange(const FRectangle& AllottedBounds)
{
    if (!Content)
    {
        return;
    }

    FRectangle LabelBounds = AllottedBounds;
    LabelBounds.Position.X += BoxSize + LabelSpacing;
    LabelBounds.Width = Math::Max(AllottedBounds.Width - BoxSize - LabelSpacing, 0);

    Content->Tick(FRectangle::AlignInBounds(LabelBounds, Content->GetCachedDesiredSize(), EHorizontalAlignment::Left, EVerticalAlignment::Center));
}

int32 FCheckBox::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style = FUIStyle::GetDefault();
    const EInteractionState State = GetInteractionState();

    const FRectangle Box = FRectangle::AlignInBounds(AllottedGeometry.Bounds, IntVector2(BoxSize, BoxSize), EHorizontalAlignment::Left, EVerticalAlignment::Center);

    const FCornerRadii BoxRadii = FCornerRadii(Math::Min(Style.Metrics.CornerRadius, static_cast<float>(BoxSize) * 0.25f));

    FFloatColor BorderColor = Style.CheckBox.Border;
    if (State == EInteractionState::Pressed)
    {
        BorderColor = Style.CheckBox.BorderPressed;
    }
    else if (State == EInteractionState::Hovered)
    {
        BorderColor = Style.CheckBox.BorderHovered;
    }

    OutCommandList.AddBox(LayerId, Box, Style.CheckBox.Fill, BoxRadii);
    OutCommandList.AddBoxOutline(LayerId, Box, BorderColor, Style.CheckBox.BorderThickness, BoxRadii);

    const FFloatColor MarkColor = IsEnabled() ? Style.CheckBox.CheckMark : Style.Colors.TextDisabled;
    if (CheckState == ECheckBoxState::Checked)
    {
        const float Left   = static_cast<float>(Box.Position.X);
        const float Top    = static_cast<float>(Box.Position.Y);
        const float Extent = static_cast<float>(BoxSize);

        const Vector2 Points[] =
        {
            Vector2(Left + (Extent * 0.24f), Top + (Extent * 0.52f)),
            Vector2(Left + (Extent * 0.44f), Top + (Extent * 0.72f)),
            Vector2(Left + (Extent * 0.78f), Top + (Extent * 0.28f)),
        };

        OutCommandList.AddPolyline(LayerId, TArrayView<const Vector2>(Points, ARRAY_COUNT(Points)), MarkColor, Math::Max(Extent * 0.14f, 1.0f));
    }
    else if (CheckState == ECheckBoxState::Undetermined)
    {
        FRectangle Dash;
        Dash.Width      = Math::Max(Box.Width / 2, 1);
        Dash.Height     = Math::Max(Box.Height / 8, 1);
        Dash.Position.X = Box.Position.X + ((Box.Width - Dash.Width) / 2);
        Dash.Position.Y = Box.Position.Y + ((Box.Height - Dash.Height) / 2);

        OutCommandList.AddBox(LayerId, Dash, MarkColor);
    }

    if (LabelText)
    {
        LabelText->SetColorAndOpacity(Style.GetTextColor(State));
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

void FCheckBox::SetCheckState(ECheckBoxState InState)
{
    CheckState = InState;
}

FRectangle FCheckBox::GetBoxBounds() const
{
    return FRectangle::AlignInBounds(GetContentRectangle(), IntVector2(BoxSize, BoxSize), EHorizontalAlignment::Left, EVerticalAlignment::Center);
}

void FCheckBox::OnClicked()
{
    CheckState = GetNextState();
    OnStateChangedDelegate.ExecuteIfBound(CheckState);
}

ECheckBoxState FCheckBox::GetNextState() const
{
    if (!bIsTriState)
    {
        return CheckState == ECheckBoxState::Checked ? ECheckBoxState::Unchecked : ECheckBoxState::Checked;
    }

    switch (CheckState)
    {
        case ECheckBoxState::Unchecked:
            return ECheckBoxState::Checked;

        case ECheckBoxState::Checked:
            return ECheckBoxState::Undetermined;

        case ECheckBoxState::Undetermined:
            return ECheckBoxState::Unchecked;
    }

    return ECheckBoxState::Unchecked;
}
