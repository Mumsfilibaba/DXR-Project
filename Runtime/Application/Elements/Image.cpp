#include "Application/Elements/Image.h"
#include "Application/Draw/DrawCommandList.h"

TSharedPtr<FImage> FImage::Create(const FDesc& Desc)
{
    TSharedPtr<FImage> NewInstance = MakeSharedPtr<FImage>();
    if (NewInstance)
    {
        NewInstance->Initialize(Desc);
    }

    return NewInstance;
}

FImage::FImage()
    : FVisualElement()
    , Brush()
    , DesiredSize()
    , Tint(FFloatColor::White)
{
}

FImage::~FImage() = default;

void FImage::Initialize(const FDesc& Desc)
{
    Brush       = Desc.Brush;
    DesiredSize = Desc.DesiredSize;
    Tint        = Desc.Tint;
}

IntVector2 FImage::ComputeDesiredSize() const
{
    return DesiredSize;
}

int32 FImage::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (Brush.IsValid())
    {
        OutCommandList.AddImage(LayerId, AllottedGeometry.Bounds, Brush, Tint);
    }

    return LayerId;
}

void FImage::SetBrush(const FUIBrush& InBrush)
{
    Brush = InBrush;
}
