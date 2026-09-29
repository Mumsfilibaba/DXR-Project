#include "Application/Draw/DisclosureGlyph.h"
#include "Application/Draw/DrawCommandList.h"

void FDisclosureGlyph::Draw(FDrawCommandList& OutCommandList, int32 LayerId, const FRectangle& Bounds, bool bIsExpanded, const FFloatColor& Tint)
{
    const float Left   = static_cast<float>(Bounds.Position.X);
    const float Top    = static_cast<float>(Bounds.Position.Y);
    const float Right  = static_cast<float>(Bounds.GetRight());
    const float Bottom = static_cast<float>(Bounds.GetBottom());

    Vector2 Corners[3];
    if (bIsExpanded)
    {
        const float CenterX = (Left + Right) * 0.5f;
        Corners[0] = Vector2(Left, Top);
        Corners[1] = Vector2(Right, Top);
        Corners[2] = Vector2(CenterX, Bottom);
    }
    else
    {
        const float CenterY = (Top + Bottom) * 0.5f;
        Corners[0] = Vector2(Left, Top);
        Corners[1] = Vector2(Right, CenterY);
        Corners[2] = Vector2(Left, Bottom);
    }

    OutCommandList.AddConvexPolygon(LayerId, TArrayView<const Vector2>(Corners, 3), Tint);
}
