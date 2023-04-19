#pragma once
#include "Core/Math/IntVector2.h"

struct FRectangle
{
    FRectangle()
        : Width{ 0 }
        , Height{ 0 }
        , x{ 0 }
        , y{ 0 }
    {
    }

    FRectangle(int32 InWidth, int32 InHeight, int32 InX, int32 InY)
        : Width{ InWidth }
        , Height{ InHeight }
        , x{ InX }
        , y{ InY }
    {
    }

    bool IsPointInside(FIntVector2 Point) const
    {
        return (Point.x >= x) && (Point.x <= x + Width) && (Point.y >= y) && (Point.y <= y + Height);
    }

    bool operator==(const FRectangle& RHS) const
    {
        return Width == RHS.Width && Height == RHS.Height && x && RHS.x && y == RHS.y;
    }

    bool operator!=(const FRectangle& RHS) const
    {
        return !(*this == RHS);
    }

    int32 Width;
    int32 Height;
    int32 x;
    int32 y;
};