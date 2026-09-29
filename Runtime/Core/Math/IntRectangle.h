#pragma once
#include "Core/CoreTypes.h"
#include "Core/Math/IntVector2.h"
#include "Core/Math/Math.h"

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
     * @brief Checks if the rectangle encapsulates a given point. The right and bottom edges are exclusive, so two
     * rectangles that share an edge never both claim the pixel on it.
     * 
     * @param Point The point to check.
     * @return True if the point is within the rectangle; false otherwise.
     */
    NODISCARD FORCEINLINE bool EncapsulatesPoint(const IntVector2& Point) const
    {
        return Point.X >= Position.X && Point.Y >= Position.Y && Point.X < GetRight() && Point.Y < GetBottom();
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
     * @brief Returns the smallest rectangle that holds both, where an empty rectangle adds nothing.
     *
     * @param Other The rectangle to grow by.
     * @return The union's bounding rectangle.
     */
    NODISCARD FORCEINLINE FRectangle Union(const FRectangle& Other) const
    {
        if (IsEmpty())
        {
            return Other;
        }

        if (Other.IsEmpty())
        {
            return *this;
        }

        const int32 MinX = Math::Min(Position.X, Other.Position.X);
        const int32 MinY = Math::Min(Position.Y, Other.Position.Y);
        const int32 MaxX = Math::Max(GetRight(), Other.GetRight());
        const int32 MaxY = Math::Max(GetBottom(), Other.GetBottom());
        return FRectangle(IntVector2(MinX, MinY), MaxX - MinX, MaxY - MinY);
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
