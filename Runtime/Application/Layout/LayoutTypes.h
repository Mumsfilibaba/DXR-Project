#pragma once
#include "Core/CoreTypes.h"
#include "Core/Math/IntRectangle.h"
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

struct FLayout
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
};

template<EOrientation Orientation>
struct TLayoutAxis
{
    static constexpr bool bIsVertical = Orientation == EOrientation::Vertical;

    /** @return The component of a vector along the stacking axis. */
    NODISCARD static FORCEINLINE int32 Main(const IntVector2& Vector)
    {
        return bIsVertical ? Vector.Y : Vector.X;
    }

    /** @return The component of a vector across the stacking axis. */
    NODISCARD static FORCEINLINE int32 Cross(const IntVector2& Vector)
    {
        return bIsVertical ? Vector.X : Vector.Y;
    }

    /** @return Where a rectangle starts along the stacking axis. */
    NODISCARD static FORCEINLINE int32 MainStart(const FRectangle& Rectangle)
    {
        return bIsVertical ? Rectangle.Position.Y : Rectangle.Position.X;
    }

    /** @return How long a rectangle is along the stacking axis. */
    NODISCARD static FORCEINLINE int32 MainExtent(const FRectangle& Rectangle)
    {
        return bIsVertical ? Rectangle.Height : Rectangle.Width;
    }

    /** @return The space a margin takes along the stacking axis. */
    NODISCARD static FORCEINLINE int32 MainTotal(const FMargin& Margin)
    {
        return bIsVertical ? Margin.GetTotalVertical() : Margin.GetTotalHorizontal();
    }

    /** @return The space a margin takes across the stacking axis. */
    NODISCARD static FORCEINLINE int32 CrossTotal(const FMargin& Margin)
    {
        return bIsVertical ? Margin.GetTotalHorizontal() : Margin.GetTotalVertical();
    }

    /** @return A size from its extent along the stacking axis and its extent across it. */
    NODISCARD static FORCEINLINE IntVector2 MakeSize(int32 MainExtent, int32 CrossExtent)
    {
        return bIsVertical ? IntVector2(CrossExtent, MainExtent) : IntVector2(MainExtent, CrossExtent);
    }

    /**
     * @brief Cuts one slot out of a rectangle, spanning the whole rectangle across the stacking axis.
     *
     * @param Bounds The rectangle the slots are stacked in.
     * @param Start  Where the slot starts along the stacking axis.
     * @param Extent How long the slot is along the stacking axis.
     * @return The slot.
     */
    NODISCARD static FORCEINLINE FRectangle MakeSlot(const FRectangle& Bounds, int32 Start, int32 Extent)
    {
        return bIsVertical
            ? FRectangle(IntVector2(Bounds.Position.X, Start), Bounds.Width, Extent)
            : FRectangle(IntVector2(Start, Bounds.Position.Y), Extent, Bounds.Height);
    }
};
