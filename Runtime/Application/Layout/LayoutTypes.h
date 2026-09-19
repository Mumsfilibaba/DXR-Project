#pragma once
#include "Core/CoreTypes.h"
#include "Core/Math/IntVector2.h"
#include "Core/Math/Math.h"

enum class EHorizontalAlignment : uint8
{
    Fill,
    Left,
    Center,
    Right,
};

enum class EVerticalAlignment : uint8
{
    Fill,
    Top,
    Center,
    Bottom,
};

enum class EOrientation : uint8
{
    Horizontal,
    Vertical,
};

enum class ETextOverflow : uint8
{
    Overflow,
    Elide,
};

struct FMargin
{
    FMargin()
        : Left(0)
        , Top(0)
        , Right(0)
        , Bottom(0)
    {
    }

    explicit FMargin(int32 UniformAmount)
        : Left(UniformAmount)
        , Top(UniformAmount)
        , Right(UniformAmount)
        , Bottom(UniformAmount)
    {
    }

    FMargin(int32 InHorizontal, int32 InVertical)
        : Left(InHorizontal)
        , Top(InVertical)
        , Right(InHorizontal)
        , Bottom(InVertical)
    {
    }

    FMargin(int32 InLeft, int32 InTop, int32 InRight, int32 InBottom)
        : Left(InLeft)
        , Top(InTop)
        , Right(InRight)
        , Bottom(InBottom)
    {
    }

    /** @return The left and right amounts added together, the space the margin takes horizontally. */
    NODISCARD FORCEINLINE int32 GetTotalHorizontal() const
    {
        return Left + Right;
    }

    /** @return The top and bottom amounts added together, the space the margin takes vertically. */
    NODISCARD FORCEINLINE int32 GetTotalVertical() const
    {
        return Top + Bottom;
    }

    NODISCARD FORCEINLINE bool operator==(const FMargin& Other) const
    {
        return Left == Other.Left && Top == Other.Top && Right == Other.Right && Bottom == Other.Bottom;
    }

    NODISCARD FORCEINLINE bool operator!=(const FMargin& Other) const
    {
        return !(*this == Other);
    }

    int32 Left;
    int32 Top;
    int32 Right;
    int32 Bottom;
};

struct FRectangle
{
    /**
     * @brief Places a child of a known size inside a rectangle. Fill on an axis hands the child the whole
     * extent, and every other value shrinks the child to the size it asked for and moves it to the named edge,
     * never growing it past the space available.
     *
     * @param Bounds              The rectangle to place the child in.
     * @param DesiredSize         The size the child asked for.
     * @param HorizontalAlignment How the child is placed on the horizontal axis.
     * @param VerticalAlignment   How the child is placed on the vertical axis.
     * @return The rectangle the child occupies.
     */
    NODISCARD static FORCEINLINE FRectangle AlignInBounds(
        const FRectangle&    Bounds,
        const IntVector2&    DesiredSize,
        EHorizontalAlignment HorizontalAlignment,
        EVerticalAlignment   VerticalAlignment)
    {
        FRectangle Result = Bounds;

        switch (HorizontalAlignment)
        {
            case EHorizontalAlignment::Left:
            {
                Result.Width = Math::Min(DesiredSize.X, Bounds.Width);
                break;
            }
            case EHorizontalAlignment::Center:
            {
                Result.Width      = Math::Min(DesiredSize.X, Bounds.Width);
                Result.Position.X = Bounds.Position.X + ((Bounds.Width - Result.Width) / 2);
                break;
            }
            case EHorizontalAlignment::Right:
            {
                Result.Width      = Math::Min(DesiredSize.X, Bounds.Width);
                Result.Position.X = Bounds.GetRight() - Result.Width;
                break;
            }
            default:
            {
                break;
            }
        }

        switch (VerticalAlignment)
        {
            case EVerticalAlignment::Top:
            {
                Result.Height = Math::Min(DesiredSize.Y, Bounds.Height);
                break;
            }
            case EVerticalAlignment::Center:
            {
                Result.Height     = Math::Min(DesiredSize.Y, Bounds.Height);
                Result.Position.Y = Bounds.Position.Y + ((Bounds.Height - Result.Height) / 2);
                break;
            }
            case EVerticalAlignment::Bottom:
            {
                Result.Height     = Math::Min(DesiredSize.Y, Bounds.Height);
                Result.Position.Y = Bounds.GetBottom() - Result.Height;
                break;
            }
            default:
            {
                break;
            }
        }

        return Result;
    }

    /** @brief Default constructor initializes width and height to zero. */
    FRectangle()
        : Width(0)
        , Height(0)
        , Position()
    {
    }

    FRectangle(const IntVector2& InPosition, int32 InWidth, int32 InHeight)
        : Width(InWidth)
        , Height(InHeight)
        , Position(InPosition)
    {
    }

    /**
     * @brief Checks if the rectangle encapsulates a given point.
     * 
     * @param Point The point to check.
     * @return True if the point is within the rectangle; false otherwise.
     */
    NODISCARD FORCEINLINE bool EncapsulatesPoint(const IntVector2& Point) const
    {
        return Point.X >= Position.X && Point.Y >= Position.Y && Point.X <= (Position.X + Width) && Point.Y <= (Position.Y + Height);
    }

    /** @return The coordinate one past the right edge, which is the left edge plus the width. */
    NODISCARD FORCEINLINE int32 GetRight() const
    {
        return Position.X + Width;
    }

    /** @return The coordinate one past the bottom edge, which is the top edge plus the height. */
    NODISCARD FORCEINLINE int32 GetBottom() const
    {
        return Position.Y + Height;
    }

    /** @return True when either extent is zero or negative, so the rectangle covers no pixels. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Width <= 0 || Height <= 0;
    }

    /** @return The center of the rectangle, with each half rounded down by the integer division. */
    NODISCARD FORCEINLINE IntVector2 GetCenter() const
    {
        return IntVector2(Position.X + (Width / 2), Position.Y + (Height / 2));
    }

    /** @return The extent of the rectangle, the width and the height without the position. */
    NODISCARD FORCEINLINE IntVector2 GetSize() const
    {
        return IntVector2(Width, Height);
    }

    /**
     * @brief Returns the rectangle shrunk on every side by the margin, never smaller than zero.
     * 
     * @param Margin The amount to shrink by on each side.
     * @return The deflated rectangle.
     */
    NODISCARD FORCEINLINE FRectangle Deflate(const FMargin& Margin) const
    {
        FRectangle Result;
        Result.Position.X = Position.X + Margin.Left;
        Result.Position.Y = Position.Y + Margin.Top;
        Result.Width      = Math::Max(0, Width - Margin.GetTotalHorizontal());
        Result.Height     = Math::Max(0, Height - Margin.GetTotalVertical());
        return Result;
    }

    /**
     * @brief Returns the rectangle grown on every side by the margin.
     *
     * @param Margin The amount to grow by on each side.
     * @return The inflated rectangle.
     */
    NODISCARD FORCEINLINE FRectangle Inflate(const FMargin& Margin) const
    {
        FRectangle Result;
        Result.Position.X = Position.X - Margin.Left;
        Result.Position.Y = Position.Y - Margin.Top;
        Result.Width      = Width + Margin.GetTotalHorizontal();
        Result.Height     = Height + Margin.GetTotalVertical();
        return Result;
    }

    /**
     * @brief Returns the overlap of the two rectangles, or an empty rectangle when they do not touch.
     *
     * @param Other The rectangle to intersect with.
     * @return The overlapping region.
     */
    NODISCARD FORCEINLINE FRectangle Intersect(const FRectangle& Other) const
    {
        const int32 MinX = Math::Max(Position.X, Other.Position.X);
        const int32 MinY = Math::Max(Position.Y, Other.Position.Y);
        const int32 MaxX = Math::Min(GetRight(), Other.GetRight());
        const int32 MaxY = Math::Min(GetBottom(), Other.GetBottom());

        FRectangle Result;
        Result.Position.X = MinX;
        Result.Position.Y = MinY;
        Result.Width      = Math::Max(0, MaxX - MinX);
        Result.Height     = Math::Max(0, MaxY - MinY);
        return Result;
    }

    /**
     * @brief Equality operator.
     * 
     * @param Other The rectangle to compare with.
     * @return True if both rectangles are equal; false otherwise.
     */
    NODISCARD FORCEINLINE bool operator==(const FRectangle& Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position == Other.Position;
    }

    /**
     * @brief Inequality operator.
     * 
     * @param Other The rectangle to compare with.
     * @return True if rectangles are not equal; false otherwise.
     */
    NODISCARD FORCEINLINE bool operator!=(const FRectangle& Other) const
    {
        return !(*this == Other);
    }

    /** @brief Width of the rectangle. */
    int32 Width;

    /** @brief Height of the rectangle. */
    int32 Height;

    /** @brief Position of the rectangle (x, y). */
    IntVector2 Position;
};
