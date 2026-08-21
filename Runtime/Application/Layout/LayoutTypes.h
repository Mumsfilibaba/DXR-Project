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

    /** @brief The space the margin takes on the horizontal axis. */
    NODISCARD FORCEINLINE int32 GetTotalHorizontal() const
    {
        return Left + Right;
    }

    /** @brief The space the margin takes on the vertical axis. */
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
     * @brief Checks if the rectangle encapsulates a given point.
     * 
     * @param Point The point to check.
     * @return True if the point is within the rectangle; false otherwise.
     */
    NODISCARD FORCEINLINE bool EncapsulatesPoint(const IntVector2& Point) const
    {
        return Point.X >= Position.X && Point.Y >= Position.Y && Point.X <= (Position.X + Width) && Point.Y <= (Position.Y + Height);
    }

    /** @brief The coordinate one past the right edge. */
    NODISCARD FORCEINLINE int32 GetRight() const
    {
        return Position.X + Width;
    }

    /** @brief The coordinate one past the bottom edge. */
    NODISCARD FORCEINLINE int32 GetBottom() const
    {
        return Position.Y + Height;
    }

    /** @brief True when the rectangle covers no pixels. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Width <= 0 || Height <= 0;
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
