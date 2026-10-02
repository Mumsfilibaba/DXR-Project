#include "Application/Menus/PopupPlacement.h"
#include "Core/Math/Math.h"

FRectangle FPopupPlacement::Resolve(
    const FRectangle& Anchor,
    const IntVector2& Size,
    const FRectangle& Area,
    EPopupSide        Side,
    EPopupFlip        Flips,
    int32             Gap,
    int32             CrossOffset)
{
    const bool bMayFlipMain  = (Flips & EPopupFlip::Main) != EPopupFlip::None;
    const bool bMayFlipCross = (Flips & EPopupFlip::Cross) != EPopupFlip::None;

    FRectangle Result(IntVector2(), Size.X, Size.Y);

    if (Side == EPopupSide::Below)
    {
        Result.Position = IntVector2(Anchor.Position.X + CrossOffset, Anchor.GetBottom() + Gap);

        const int32 AboveY = Anchor.Position.Y - Gap - Size.Y;
        if (bMayFlipMain && Result.GetBottom() > Area.GetBottom() && AboveY >= Area.Position.Y)
        {
            Result.Position.Y = AboveY;
        }

        const int32 RightAlignedX = Anchor.GetRight() - Size.X - CrossOffset;
        if (bMayFlipCross && Result.GetRight() > Area.GetRight() && RightAlignedX >= Area.Position.X)
        {
            Result.Position.X = RightAlignedX;
        }
    }
    else
    {
        Result.Position = IntVector2(Anchor.GetRight() + Gap, Anchor.Position.Y + CrossOffset);

        const int32 LeftX = Anchor.Position.X - Gap - Size.X;
        if (bMayFlipMain && Result.GetRight() > Area.GetRight() && LeftX >= Area.Position.X)
        {
            Result.Position.X = LeftX;
        }

        const int32 BottomAlignedY = Anchor.GetBottom() - Size.Y - CrossOffset;
        if (bMayFlipCross && Result.GetBottom() > Area.GetBottom() && BottomAlignedY >= Area.Position.Y)
        {
            Result.Position.Y = BottomAlignedY;
        }
    }

    return ClampIntoArea(Result, Area);
}

FRectangle FPopupPlacement::ClampIntoArea(const FRectangle& Bounds, const FRectangle& Area)
{
    FRectangle Result = Bounds;
    Result.Position.X = Math::Clamp(Result.Position.X, Area.Position.X, Math::Max(Area.Position.X, Area.GetRight() - Bounds.Width));
    Result.Position.Y = Math::Clamp(Result.Position.Y, Area.Position.Y, Math::Max(Area.Position.Y, Area.GetBottom() - Bounds.Height));
    return Result;
}
