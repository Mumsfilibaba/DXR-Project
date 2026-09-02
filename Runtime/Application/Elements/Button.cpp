#include "Application/Elements/Button.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FButton> FButton::Create(const FDesc& Desc)
{
    TSharedPtr<FButton> NewButton = MakeSharedPtr<FButton>();
    NewButton->Initialize(Desc);
    return NewButton;
}

FButton::FButton()
    : FInteractiveElement()
    , Label(nullptr)
    , CornerRadius()
    , HorizontalContentAlignment(EHorizontalAlignment::Center)
    , VerticalContentAlignment(EVerticalAlignment::Center)
    , MinHeight(0)
    , bHasBorder(false)
    , OnClickedDelegate()
{
}

FButton::~FButton() = default;

void FButton::Initialize(const FDesc& Desc)
{
    CornerRadius               = Desc.CornerRadius;
    HorizontalContentAlignment = Desc.HorizontalContentAlignment;
    VerticalContentAlignment   = Desc.VerticalContentAlignment;
    MinHeight                  = Desc.MinHeight;
    bHasBorder                 = Desc.bHasBorder;
    OnClickedDelegate          = Desc.OnClicked;

    SetPadding(Desc.Padding);

    if (Desc.Content)
    {
        SetContent(Desc.Content);
        return;
    }

    FTextBlock::FDesc LabelDesc;
    LabelDesc.Text            = Desc.Text;
    LabelDesc.Font            = Desc.Font;
    LabelDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    Label = FTextBlock::Create(LabelDesc);
    SetContent(Label);
}

IntVector2 FButton::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = FCompoundElement::ComputeDesiredSize();
    DesiredSize.Y          = Math::Max(DesiredSize.Y, MinHeight);
    return DesiredSize;
}

void FButton::OnArrange(const FRectangle& AllottedBounds)
{
    if (!Content)
    {
        return;
    }

    const FRectangle Available = AllottedBounds.Deflate(GetPadding());
    Content->Tick(FRectangle::AlignInBounds(Available, Content->GetCachedDesiredSize(), HorizontalContentAlignment, VerticalContentAlignment));
}

int32 FButton::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style = FUIStyle::GetDefault();
    const EInteractionState State = GetInteractionState();

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.GetButtonColor(State, false), CornerRadius);

    if (bHasBorder)
    {
        OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, Style.Colors.Border, Style.Metrics.BorderThickness, CornerRadius);
    }

    if (Label)
    {
        Label->SetColorAndOpacity(Style.GetTextColor(State));
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

void FButton::SetOnClicked(const FOnClicked& InOnClicked)
{
    OnClickedDelegate = InOnClicked;
}

void FButton::SetText(const String& InText)
{
    if (Label)
    {
        Label->SetText(InText);
    }
}

const String& FButton::GetText() const
{
    static const String EmptyText;
    return Label ? Label->GetText() : EmptyText;
}

void FButton::OnClicked()
{
    OnClickedDelegate.ExecuteIfBound();
}
