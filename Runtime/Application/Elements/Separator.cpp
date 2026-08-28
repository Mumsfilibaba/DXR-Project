#include "Application/Elements/Separator.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FSeparator> FSeparator::Create(const FDesc& Desc)
{
    TSharedPtr<FSeparator> NewSeparator = MakeSharedPtr<FSeparator>();
    NewSeparator->Initialize(Desc);
    return NewSeparator;
}

TSharedPtr<FSeparator> FSeparator::CreateHorizontal()
{
    return Create(FDesc());
}

TSharedPtr<FSeparator> FSeparator::CreateVertical()
{
    FDesc Desc;
    Desc.Orientation = EOrientation::Vertical;
    return Create(Desc);
}

FSeparator::FSeparator()
    : FVisualElement()
    , Orientation(EOrientation::Horizontal)
    , Thickness(1)
    , Padding()
    , Color(FFloatColor::White)
{
}

FSeparator::~FSeparator() = default;

void FSeparator::Initialize(const FDesc& Desc)
{
    Orientation = Desc.Orientation;
    Thickness   = Math::Max(Desc.Thickness, 1);
    Padding     = Desc.Padding;
    Color       = Desc.Color;
}

IntVector2 FSeparator::ComputeDesiredSize() const
{
    return Orientation == EOrientation::Horizontal
        ? IntVector2(Padding.GetTotalHorizontal(), Thickness + Padding.GetTotalVertical())
        : IntVector2(Thickness + Padding.GetTotalHorizontal(), Padding.GetTotalVertical());
}

int32 FSeparator::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle Available = AllottedGeometry.Bounds.Deflate(Padding);

    FRectangle Line = Available;
    if (Orientation == EOrientation::Horizontal)
    {
        Line.Height     = Math::Min(Thickness, Available.Height);
        Line.Position.Y = Available.Position.Y + ((Available.Height - Line.Height) / 2);
    }
    else
    {
        Line.Width      = Math::Min(Thickness, Available.Width);
        Line.Position.X = Available.Position.X + ((Available.Width - Line.Width) / 2);
    }

    if (!Line.IsEmpty())
    {
        OutCommandList.AddBox(LayerId, Line, Color);
    }

    return LayerId;
}
