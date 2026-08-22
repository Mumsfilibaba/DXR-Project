#include "Application/Elements/BorderElement.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

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
    , CornerRadius(0.0f)
    , MinHeight(0)
    , Cursor(ECursor::None)
    , bHasCursor(false)
{
}

FBorderElement::~FBorderElement() = default;

void FBorderElement::Initialize(const FInitializer& Initializer)
{
    BackgroundColor = Initializer.BackgroundColor;
    CornerRadius    = Initializer.CornerRadius;
    MinHeight       = Initializer.MinHeight;
    Cursor          = Initializer.Cursor;
    bHasCursor      = Initializer.bHasCursor;
    SetPadding(Initializer.Padding);
    SetContent(Initializer.Content);
}

IntVector2 FBorderElement::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = FCompoundElement::ComputeDesiredSize();
    DesiredSize.Y          = Math::Max(DesiredSize.Y, MinHeight);
    return DesiredSize;
}

int32 FBorderElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (BackgroundColor.A > 0.0f)
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, BackgroundColor, CornerRadius);
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

void FBorderElement::SetBackgroundColor(const FFloatColor& InBackgroundColor)
{
    BackgroundColor = InBackgroundColor;
}

void FBorderElement::SetCornerRadius(float InCornerRadius)
{
    CornerRadius = InCornerRadius;
}

bool FBorderElement::GetCursor(ECursor& OutCursor) const
{
    if (!bHasCursor)
    {
        return false;
    }

    OutCursor = Cursor;
    return true;
}

void FBorderElement::SetMinHeight(int32 InMinHeight)
{
    MinHeight = InMinHeight;
}

void FBorderElement::SetCursor(ECursor InCursor)
{
    Cursor     = InCursor;
    bHasCursor = true;
}

void FBorderElement::ClearCursor()
{
    Cursor     = ECursor::None;
    bHasCursor = false;
}
