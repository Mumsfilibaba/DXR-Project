#include "Application/Elements/BorderElement.h"
#include "Application/Draw/DrawCommandList.h"

TSharedPtr<FBorderElement> FBorderElement::Create(const FInitializer& Initializer)
{
    TSharedPtr<FBorderElement> NewElement = MakeSharedPtr<FBorderElement>();
    if (NewElement)
    {
        NewElement->Initialize(Initializer);
    }

    return NewElement;
}

FBorderElement::FBorderElement()
    : FCompoundElement()
    , BackgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
{
}

FBorderElement::~FBorderElement() = default;

void FBorderElement::Initialize(const FInitializer& Initializer)
{
    BackgroundColor = Initializer.BackgroundColor;
    SetPadding(Initializer.Padding);
    SetContent(Initializer.Content);
}

int32 FBorderElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (BackgroundColor.A > 0.0f)
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, BackgroundColor);
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

void FBorderElement::SetBackgroundColor(const FFloatColor& InBackgroundColor)
{
    BackgroundColor = InBackgroundColor;
}
