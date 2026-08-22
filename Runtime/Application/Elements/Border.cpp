#include "Application/Elements/Border.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

TSharedPtr<FBorder> FBorder::Create(const FDesc& Desc)
{
    TSharedPtr<FBorder> NewInstance = MakeSharedPtr<FBorder>();
    if (NewInstance)
    {
        NewInstance->Initialize(Desc);
    }

    return NewInstance;
}

FBorder::FBorder()
    : FCompoundElement()
    , BackgroundColor(0.0f, 0.0f, 0.0f, 0.0f)
    , CornerRadius(0.0f)
    , MinHeight(0)
    , Cursor(ECursor::None)
    , bHasCursor(false)
{
}

FBorder::~FBorder() = default;

void FBorder::Initialize(const FDesc& Desc)
{
    BackgroundColor = Desc.BackgroundColor;
    CornerRadius    = Desc.CornerRadius;
    MinHeight       = Desc.MinHeight;
    Cursor          = Desc.Cursor;
    bHasCursor      = Desc.bHasCursor;
    SetPadding(Desc.Padding);
    SetContent(Desc.Content);
}

IntVector2 FBorder::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = FCompoundElement::ComputeDesiredSize();
    DesiredSize.Y          = Math::Max(DesiredSize.Y, MinHeight);
    return DesiredSize;
}

int32 FBorder::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (BackgroundColor.A > 0.0f)
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, BackgroundColor, CornerRadius);
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

void FBorder::SetBackgroundColor(const FFloatColor& InBackgroundColor)
{
    BackgroundColor = InBackgroundColor;
}

void FBorder::SetCornerRadius(float InCornerRadius)
{
    CornerRadius = InCornerRadius;
}

bool FBorder::GetCursor(ECursor& OutCursor) const
{
    if (!bHasCursor)
    {
        return false;
    }

    OutCursor = Cursor;
    return true;
}

void FBorder::SetMinHeight(int32 InMinHeight)
{
    MinHeight = InMinHeight;
}

void FBorder::SetCursor(ECursor InCursor)
{
    Cursor     = InCursor;
    bHasCursor = true;
}

void FBorder::ClearCursor()
{
    Cursor     = ECursor::None;
    bHasCursor = false;
}
