#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>

#include "NavButton.h"

TSharedPtr<FNavButton> FNavButton::Create(const FDesc& Desc)
{
    TSharedPtr<FNavButton> NewButton = MakeSharedPtr<FNavButton>();
    NewButton->Initialize(Desc);
    return NewButton;
}

FNavButton::FNavButton()
    : FInteractiveElement()
    , Label(nullptr)
    , OnClickedDelegate()
    , Index(0)
    , bIsSelected(false)
{
}

FNavButton::~FNavButton()
{
}

void FNavButton::Initialize(const FDesc& Desc)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    Index = Desc.Index;

    FTextBlock::FDesc LabelDesc;
    LabelDesc.Text            = Desc.Label;
    LabelDesc.Font            = Desc.Font;
    LabelDesc.ColorAndOpacity = Style.Colors.Text;

    Label = FTextBlock::Create(LabelDesc);

    SetPadding(FMargin(12, 5, 12, 5));
    SetContent(Label);
}

int32 FNavButton::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style = FUIStyle::GetDefault();
    const EInteractionState State = GetInteractionState();

    FFloatColor Fill = bIsSelected ? Style.Colors.Accent : Style.GetControlColor(State);
    if (bIsSelected)
    {
        if (State == EInteractionState::Hovered)
        {
            Fill = Fill + 0.08f;
        }
        else if (State == EInteractionState::Pressed)
        {
            Fill = Fill - 0.08f;
        }
    }

    Fill.A = bIsSelected ? 1.0f : Fill.A;

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Fill, Style.Metrics.CornerRadius);

    if (Label)
    {
        Label->SetColorAndOpacity(Style.GetTextColor(State));
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);
}

void FNavButton::SetSelected(bool bInIsSelected)
{
    bIsSelected = bInIsSelected;
}

void FNavButton::SetOnClicked(const FOnNavButtonClicked& InOnClicked)
{
    OnClickedDelegate = InOnClicked;
}

void FNavButton::OnClicked()
{
    OnClickedDelegate.ExecuteIfBound(Index);
}
