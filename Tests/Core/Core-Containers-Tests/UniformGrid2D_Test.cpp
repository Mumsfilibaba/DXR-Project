#include "UniformGrid2D_Test.h"

#if RUN_UNIFORMGRID2D_TEST
#include "TestUtils.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/UniformGrid2D.h>

static TArray<int32> CollectCell(const FUniformGrid2D& Grid, const IntVector2& Point)
{
    TArray<int32> Items;
    Grid.ForEachInCell(Point, [&Items](int32 Index)
    {
        Items.Add(Index);
    });

    return Items;
}

bool UniformGrid2D_Test()
{
    TEST_BEGIN();

    const TArray<FRectangle> ItemBounds =
    {
        FRectangle(IntVector2(0, 0), 10, 10),
        FRectangle(IntVector2(50, 50), 100, 20),
        FRectangle(IntVector2(5, 5), 60, 60),
        FRectangle(),
    };

    FUniformGrid2D Grid;
    Grid.Build(FRectangle(IntVector2(0, 0), 200, 200), 32, ItemBounds);

    TEST_SECTION("A point lists every item overlapping its cell, in increasing order");
    const TArray<int32> Corner = CollectCell(Grid, IntVector2(2, 2));
    TEST_EXPECT(Corner == TArray<int32>({ 0, 2 }));

    TEST_SECTION("An item spanning several cells is found from each of them");
    TEST_EXPECT(CollectCell(Grid, IntVector2(140, 60)).Contains(1));
    TEST_EXPECT(CollectCell(Grid, IntVector2(52, 52)).Contains(1));
    TEST_EXPECT(CollectCell(Grid, IntVector2(52, 52)).Contains(2));

    TEST_SECTION("An empty item is never listed");
    for (int32 Y = 0; Y < 200; Y += 16)
    {
        for (int32 X = 0; X < 200; X += 16)
        {
            TEST_EXPECT(!CollectCell(Grid, IntVector2(X, Y)).Contains(3));
        }
    }

    TEST_SECTION("A point outside the indexed area finds nothing");
    TEST_EXPECT(CollectCell(Grid, IntVector2(-1, 10)).IsEmpty());
    TEST_EXPECT(CollectCell(Grid, IntVector2(200, 10)).IsEmpty());

    TEST_SECTION("A cell with nothing in it finds nothing");
    TEST_EXPECT(CollectCell(Grid, IntVector2(190, 190)).IsEmpty());

    TEST_SECTION("A huge area is cut into no more than the cell limit along either axis");
    FUniformGrid2D HugeGrid;
    HugeGrid.Build(FRectangle(IntVector2(0, 0), 100000, 10), 1, TArrayView<const FRectangle>(ItemBounds.Data(), 1));
    TEST_EXPECT(CollectCell(HugeGrid, IntVector2(3, 3)).Contains(0));

    TEST_SECTION("Reset empties the grid");
    Grid.Reset();
    TEST_EXPECT(Grid.IsEmpty());
    TEST_EXPECT(CollectCell(Grid, IntVector2(2, 2)).IsEmpty());

    TEST_END();
}

#endif
