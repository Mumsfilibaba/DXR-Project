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
    , BorderColor(0.0f, 0.0f, 0.0f, 0.0f)
    , CornerRadius()
    , BorderThickness(0.0f)
    , MinWidth(0)
    , MinHeight(0)
    , Cursor(ECursor::None)
    , bHasCursor(false)
    , bDrawBorderOverContent(false)
{
}

FBorder::~FBorder() = default;

void FBorder::Initialize(const FDesc& Desc)
{
    BackgroundColor = Desc.BackgroundColor;
    BorderColor     = Desc.BorderColor;
    CornerRadius    = Desc.CornerRadius;
    BorderThickness = Desc.BorderThickness;
    MinWidth        = Desc.MinWidth;
    MinHeight       = Desc.MinHeight;
    Cursor          = Desc.Cursor;
    bHasCursor      = Desc.bHasCursor;

    bDrawBorderOverContent = Desc.bDrawBorderOverContent;

    SetPadding(Desc.Padding);
    SetContent(Desc.Content);
}

IntVector2 FBorder::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = FCompoundElement::ComputeDesiredSize();
    DesiredSize.X          = Math::Max(DesiredSize.X, MinWidth);
    DesiredSize.Y          = Math::Max(DesiredSize.Y, MinHeight);
    return DesiredSize;
}

int32 FBorder::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (BackgroundColor.A > 0.0f)
    {
        OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, BackgroundColor, CornerRadius);
    }

    const bool bDrawsStroke = BorderColor.A > 0.0f && BorderThickness > 0.0f;

    if (bDrawsStroke && !bDrawBorderOverContent)
    {
        OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, BorderColor, BorderThickness, CornerRadius);
    }

    const int32 ContentLayerId = FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);

    if (bDrawsStroke && bDrawBorderOverContent)
    {
        OutCommandList.AddBoxOutline(ContentLayerId, AllottedGeometry.Bounds, BorderColor, BorderThickness, CornerRadius);
        return ContentLayerId + 1;
    }

    return ContentLayerId;
}

void FBorder::SetBackgroundColor(const FFloatColor& InBackgroundColor)
{
    BackgroundColor = InBackgroundColor;
}

void FBorder::SetBorderColor(const FFloatColor& InBorderColor)
{
    BorderColor = InBorderColor;
}

void FBorder::SetCornerRadius(const FCornerRadii& InCornerRadius)
{
    CornerRadius = InCornerRadius;
}

void FBorder::SetOuterCornerRadius(float InCornerRadius)
{
    CornerRadius = FCornerRadii(InCornerRadius);
}

void FBorder::SetDrawBorderOverContent(bool bInDrawBorderOverContent)
{
    bDrawBorderOverContent = bInDrawBorderOverContent;
}

void FBorder::SetBorderThickness(float InBorderThickness)
{
    BorderThickness = InBorderThickness;
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

void FBorder::SetMinWidth(int32 InMinWidth)
{
    MinWidth = InMinWidth;
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
