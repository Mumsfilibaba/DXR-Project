#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Math/IntRectangle.h"
#include "Core/Math/Math.h"

class FUniformGrid2D
{
public:
    /** @brief The most cells a grid is cut into along either axis, which the cell size grows to respect. */
    static constexpr int32 MaxCellsPerAxis = 128;

    FUniformGrid2D()
        : Bounds()
        , CellSize(1)
        , NumColumns(0)
        , NumRows(0)
        , CellStarts()
        , CellItems()
    {
    }

    /** @brief Drops every item, keeping the allocations for the next build. */
    void Reset()
    {
        Bounds     = FRectangle();
        NumColumns = 0;
        NumRows    = 0;
        CellStarts.Clear();
        CellItems.Clear();
    }

    /**
     * @brief Indexes a set of rectangles.
     *
     * @param InBounds   The area to index, which a query outside of finds nothing in.
     * @param InCellSize The edge of one cell, in the same units as the rectangles, enlarged when the area would need
     * more than MaxCellsPerAxis cells along an axis.
     * @param ItemBounds One rectangle per item. Empty rectangles are left out of every cell.
     */
    void Build(const FRectangle& InBounds, int32 InCellSize, TArrayView<const FRectangle> ItemBounds)
    {
        Reset();

        if (InBounds.IsEmpty())
        {
            return;
        }

        const int32 LongestExtent = Math::Max(InBounds.Width, InBounds.Height);

        Bounds     = InBounds;
        CellSize   = Math::Max(Math::Max(InCellSize, 1), (LongestExtent + MaxCellsPerAxis - 1) / MaxCellsPerAxis);
        NumColumns = (InBounds.Width + CellSize - 1) / CellSize;
        NumRows    = (InBounds.Height + CellSize - 1) / CellSize;

        const int32 NumCells = NumColumns * NumRows;
        CellStarts.Reset(NumCells + 1, 0);

        for (const FRectangle& Item : ItemBounds)
        {
            ForEachOverlappedCell(Item, [this](int32 CellIndex)
            {
                ++CellStarts[CellIndex + 1];
            });
        }

        for (int32 CellIndex = 0; CellIndex < NumCells; ++CellIndex)
        {
            CellStarts[CellIndex + 1] += CellStarts[CellIndex];
        }

        CellItems.Reset(CellStarts[NumCells], 0);

        TArray<int32> Cursors;
        Cursors.Reset(NumCells, 0);

        for (int32 ItemIndex = 0; ItemIndex < ItemBounds.Size(); ++ItemIndex)
        {
            ForEachOverlappedCell(ItemBounds[ItemIndex], [this, &Cursors, ItemIndex](int32 CellIndex)
            {
                CellItems[CellStarts[CellIndex] + Cursors[CellIndex]++] = ItemIndex;
            });
        }
    }

    /**
     * @brief Calls a functor with every item listed in the cell under a point, which includes every item whose
     * rectangle holds the point along with neighbours that merely share the cell.
     *
     * @param Point   The point to look under.
     * @param Functor Called with each item index, in increasing order.
     */
    template<typename FunctorType>
    void ForEachInCell(const IntVector2& Point, FunctorType&& Functor) const
    {
        if (NumColumns <= 0 || !Bounds.EncapsulatesPoint(Point))
        {
            return;
        }

        const int32 Column    = (Point.X - Bounds.Position.X) / CellSize;
        const int32 Row       = (Point.Y - Bounds.Position.Y) / CellSize;
        const int32 CellIndex = (Row * NumColumns) + Column;

        for (int32 Index = CellStarts[CellIndex]; Index < CellStarts[CellIndex + 1]; ++Index)
        {
            Functor(CellItems[Index]);
        }
    }

    /** @return True when the grid holds no items, which a grid that was never built does not. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return CellItems.IsEmpty();
    }

private:
    template<typename FunctorType>
    void ForEachOverlappedCell(const FRectangle& Item, FunctorType&& Functor) const
    {
        const FRectangle Clipped = Item.Intersect(Bounds);
        if (Clipped.IsEmpty())
        {
            return;
        }

        const int32 FirstColumn = (Clipped.Position.X - Bounds.Position.X) / CellSize;
        const int32 FirstRow    = (Clipped.Position.Y - Bounds.Position.Y) / CellSize;
        const int32 LastColumn  = (Clipped.GetRight() - 1 - Bounds.Position.X) / CellSize;
        const int32 LastRow     = (Clipped.GetBottom() - 1 - Bounds.Position.Y) / CellSize;

        for (int32 Row = FirstRow; Row <= LastRow; ++Row)
        {
            for (int32 Column = FirstColumn; Column <= LastColumn; ++Column)
            {
                Functor((Row * NumColumns) + Column);
            }
        }
    }

    FRectangle    Bounds;
    int32         CellSize;
    int32         NumColumns;
    int32         NumRows;
    TArray<int32> CellStarts;
    TArray<int32> CellItems;
};
