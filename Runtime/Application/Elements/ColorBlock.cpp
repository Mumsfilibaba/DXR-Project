#include "Application/Elements/ColorBlock.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"

TSharedPtr<FColorBlock> FColorBlock::Create(const FDesc& Desc)
{
    TSharedPtr<FColorBlock> NewColorBlock = MakeSharedPtr<FColorBlock>();
    NewColorBlock->Initialize(Desc);
    return NewColorBlock;
}

FColorBlock::FColorBlock()
    : FInteractiveElement()
    , Color(FFloatColor::White)
    , Extent(FUIStyle::GetDefault().Metrics.ButtonHeight)
    , OnClickedDelegate()
{
}

FColorBlock::~FColorBlock() = default;

void FColorBlock::Initialize(const FDesc& Desc)
{
    Color             = Desc.Color;
    Extent            = Math::Max(1, Desc.Extent);
    OnClickedDelegate = Desc.OnClicked;
}

IntVector2 FColorBlock::ComputeDesiredSize() const
{
    return IntVector2(Extent, Extent);
}

int32 FColorBlock::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&    Style = FUIStyle::GetDefault();
    const FCornerRadii Radii(Style.Metrics.CornerRadius);

    const FFloatColor Opaque(Color.R, Color.G, Color.B, 1.0f);
    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Opaque, Radii);

    const bool bIsLit = GetInteractionState() == EInteractionState::Hovered || GetInteractionState() == EInteractionState::Pressed;
    const FFloatColor& Border = bIsLit ? Style.Colors.InputFieldBorderHovered : Style.Colors.InputFieldBorder;

    OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, Border, Style.Metrics.BorderThickness, Radii);
    return LayerId;
}

void FColorBlock::SetColor(const FFloatColor& InColor)
{
    Color = InColor;
}

void FColorBlock::OnClicked()
{
    OnClickedDelegate.ExecuteIfBound();
}
