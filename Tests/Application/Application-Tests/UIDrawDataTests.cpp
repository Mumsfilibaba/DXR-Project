#include "UIDrawDataTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Math/Math.h>
#include <Core/Misc/Paths.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/UIDrawData.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/TrueTypeFontFace.h>

static TSharedPtr<FTrueTypeFontFace> CreateTrueTypeFont()
{
    return FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/consola.ttf", 16);
}

bool UIDrawDataLayerOrder_Test()
{
    TEST_BEGIN();

    FDrawCommandList CommandList;
    FUIDrawData      DrawData;

    TEST_SECTION("An empty list produces no geometry");
    DrawData.BuildFromCommandList(CommandList);
    TEST_EXPECT(DrawData.IsEmpty());
    TEST_EXPECT(DrawData.GetVertices().IsEmpty());
    TEST_EXPECT(DrawData.GetIndices().IsEmpty());

    TEST_SECTION("A box becomes one quad of four vertices and six indices");
    CommandList.AddBox(0, FRectangle(IntVector2(10, 20), 30, 40), FFloatColor::White);
    DrawData.BuildFromCommandList(CommandList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 6);
    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT_EQ(DrawData.GetBatches()[0].IndexCount, 6);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Atlas == nullptr);

    TEST_SECTION("The quad spans the bounds of the command");
    TEST_EXPECT(DrawData.GetVertices()[0].Position == Vector2(10.0f, 20.0f));
    TEST_EXPECT(DrawData.GetVertices()[2].Position == Vector2(40.0f, 60.0f));

    TEST_SECTION("The tint is packed with red in the low byte");
    TEST_EXPECT_EQ(DrawData.GetVertices()[0].Color, 0xffffffffu);

    FDrawCommandList RedList;
    RedList.AddBox(0, FRectangle(IntVector2(0, 0), 1, 1), FFloatColor(1.0f, 0.0f, 0.0f, 1.0f));
    DrawData.BuildFromCommandList(RedList);
    TEST_EXPECT_EQ(DrawData.GetVertices()[0].Color, 0xff0000ffu);

    TEST_SECTION("Commands are emitted in layer order rather than emission order");
    FDrawCommandList LayeredList;
    LayeredList.AddBox(5, FRectangle(IntVector2(100, 100), 10, 10), FFloatColor::White);
    LayeredList.AddBox(1, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White);
    DrawData.BuildFromCommandList(LayeredList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 8);
    TEST_EXPECT(DrawData.GetVertices()[0].Position == Vector2(0.0f, 0.0f));
    TEST_EXPECT(DrawData.GetVertices()[4].Position == Vector2(100.0f, 100.0f));

    TEST_SECTION("Two commands on one layer keep the order they were recorded in");
    FDrawCommandList TiedList;
    TiedList.AddBox(2, FRectangle(IntVector2(7, 7), 10, 10), FFloatColor::White);
    TiedList.AddBox(2, FRectangle(IntVector2(9, 9), 10, 10), FFloatColor::White);
    DrawData.BuildFromCommandList(TiedList);

    TEST_EXPECT(DrawData.GetVertices()[0].Position == Vector2(7.0f, 7.0f));
    TEST_EXPECT(DrawData.GetVertices()[4].Position == Vector2(9.0f, 9.0f));

    TEST_SECTION("An empty box contributes nothing");
    FDrawCommandList EmptyBoxList;
    EmptyBoxList.AddBox(0, FRectangle(IntVector2(0, 0), 0, 10), FFloatColor::White);
    DrawData.BuildFromCommandList(EmptyBoxList);
    TEST_EXPECT(DrawData.IsEmpty());

    TEST_SECTION("Reset drops the geometry again");
    DrawData.BuildFromCommandList(CommandList);
    DrawData.Reset();
    TEST_EXPECT(DrawData.IsEmpty());
    TEST_EXPECT(DrawData.GetVertices().IsEmpty());

    TEST_END();
}

bool UIDrawDataBatching_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;

    TEST_SECTION("Boxes under one clip region share a batch");
    FDrawCommandList CommandList;
    CommandList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    CommandList.AddBox(1, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White);
    CommandList.AddBox(2, FRectangle(IntVector2(10, 10), 10, 10), FFloatColor::White);
    CommandList.PopClip(3);

    DrawData.BuildFromCommandList(CommandList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT_EQ(DrawData.GetBatches()[0].IndexCount, 12);
    TEST_EXPECT(DrawData.GetBatches()[0].ScissorRectangle == FRectangle(IntVector2(0, 0), 50, 50));

    TEST_SECTION("Entering and leaving a clip region splits the batches");
    FDrawCommandList SplitList;
    SplitList.AddBox(0, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White);
    SplitList.PushClip(1, FRectangle(IntVector2(0, 0), 50, 50));
    SplitList.AddBox(2, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White);
    SplitList.PopClip(3);
    SplitList.AddBox(4, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White);

    DrawData.BuildFromCommandList(SplitList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 3);
    TEST_EXPECT(DrawData.GetBatches()[0].ScissorRectangle.IsEmpty());
    TEST_EXPECT(DrawData.GetBatches()[1].ScissorRectangle == FRectangle(IntVector2(0, 0), 50, 50));
    TEST_EXPECT(DrawData.GetBatches()[2].ScissorRectangle.IsEmpty());

    TEST_SECTION("The batches partition the index buffer without a gap");
    int32 ExpectedOffset = 0;
    for (const FUIDrawBatch& Batch : DrawData.GetBatches())
    {
        TEST_EXPECT_EQ(Batch.IndexOffset, ExpectedOffset);
        ExpectedOffset += Batch.IndexCount;
    }

    TEST_EXPECT_EQ(ExpectedOffset, DrawData.GetIndices().Size());

    TEST_SECTION("A batch that was never clipped is told apart from one that was");
    TEST_EXPECT(!DrawData.GetBatches()[0].bIsClipped);
    TEST_EXPECT(DrawData.GetBatches()[1].bIsClipped);
    TEST_EXPECT(!DrawData.GetBatches()[2].bIsClipped);

    TEST_SECTION("A nested region carries the intersection down to the batch");
    FDrawCommandList NestedList;
    NestedList.PushClip(0, FRectangle(IntVector2(0, 0), 100, 100));
    NestedList.PushClip(1, FRectangle(IntVector2(50, 50), 100, 100));
    NestedList.AddBox(2, FRectangle(IntVector2(60, 60), 10, 10), FFloatColor::White);
    NestedList.PopClip(3);
    NestedList.PopClip(4);

    DrawData.BuildFromCommandList(NestedList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT(DrawData.GetBatches()[0].ScissorRectangle == FRectangle(IntVector2(50, 50), 50, 50));
    TEST_EXPECT(DrawData.GetBatches()[0].bIsClipped);

    TEST_END();
}

bool UIDrawDataClipCulling_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;

    TEST_SECTION("A box that misses the active region is dropped rather than scissored away");
    FDrawCommandList CulledList;
    CulledList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    CulledList.AddBox(1, FRectangle(IntVector2(200, 200), 10, 10), FFloatColor::White);
    CulledList.PopClip(2);

    DrawData.BuildFromCommandList(CulledList);
    TEST_EXPECT(DrawData.IsEmpty());
    TEST_EXPECT(DrawData.GetBatches().IsEmpty());

    TEST_SECTION("One that only overlaps it is kept whole, since the scissor takes care of the rest");
    FDrawCommandList OverlappingList;
    OverlappingList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    OverlappingList.AddBox(1, FRectangle(IntVector2(40, 40), 100, 100), FFloatColor::White);
    OverlappingList.PopClip(2);

    DrawData.BuildFromCommandList(OverlappingList);
    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);

    TEST_SECTION("A line is culled the same way");
    FDrawCommandList LineList;
    LineList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    LineList.AddLine(1, FRectangle(IntVector2(200, 0), 1, 16), FFloatColor::White);
    LineList.PopClip(2);

    DrawData.BuildFromCommandList(LineList);
    TEST_EXPECT(DrawData.IsEmpty());

    TEST_SECTION("A region clipped to nothing draws nothing at all, rather than everything unclipped");
    FDrawCommandList EmptyClipList;
    EmptyClipList.PushClip(0, FRectangle(IntVector2(0, 0), 0, 0));
    EmptyClipList.AddBox(1, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White);
    EmptyClipList.PopClip(2);

    DrawData.BuildFromCommandList(EmptyClipList);
    TEST_EXPECT(DrawData.IsEmpty());

    TEST_SECTION("Leaving the region again lets the commands after it through");
    FDrawCommandList AfterList;
    AfterList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    AfterList.AddBox(1, FRectangle(IntVector2(200, 200), 10, 10), FFloatColor::White);
    AfterList.PopClip(2);
    AfterList.AddBox(3, FRectangle(IntVector2(200, 200), 10, 10), FFloatColor::White);

    DrawData.BuildFromCommandList(AfterList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT(!DrawData.GetBatches()[0].bIsClipped);

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();
    TEST_EXPECT(Font != nullptr);

    if (!Font)
    {
        TEST_END();
    }

    TEST_SECTION("Text off the end of a long list never reaches the atlas either");
    FDrawCommandList TextList;
    TextList.PushClip(0, FRectangle(IntVector2(0, 0), 200, 50));
    TextList.AddText(1, FRectangle(IntVector2(0, 400), 100, 16), String("Offscreen"), Font.Get(), FFloatColor::White);
    TextList.AddText(2, FRectangle(IntVector2(0, 0), 100, 16), String("Onscreen"), Font.Get(), FFloatColor::White);
    TextList.PopClip(3);

    DrawData.BuildFromCommandList(TextList);

    FDrawCommandList OnscreenOnlyList;
    OnscreenOnlyList.AddText(0, FRectangle(IntVector2(0, 0), 100, 16), String("Onscreen"), Font.Get(), FFloatColor::White);

    FUIDrawData OnscreenOnlyData;
    OnscreenOnlyData.BuildFromCommandList(OnscreenOnlyList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), OnscreenOnlyData.GetVertices().Size());

    TEST_END();
}

bool UIDrawDataSiblingClips_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;

    const FRectangle LeftRegion(IntVector2(0, 0), 100, 100);
    const FRectangle RightRegion(IntVector2(200, 0), 100, 100);

    TEST_SECTION("Two disjoint regions opened on the same layer each keep their own contents");

    FDrawCommandList SiblingList;
    SiblingList.PushClip(0, LeftRegion);
    SiblingList.AddBox(0, FRectangle(IntVector2(10, 10), 20, 20), FFloatColor::White);
    SiblingList.AddBox(1, FRectangle(IntVector2(10, 40), 20, 20), FFloatColor::White);
    SiblingList.PopClip(1);

    SiblingList.PushClip(0, RightRegion);
    SiblingList.AddBox(0, FRectangle(IntVector2(210, 10), 20, 20), FFloatColor::White);
    SiblingList.AddBox(1, FRectangle(IntVector2(210, 40), 20, 20), FFloatColor::White);
    SiblingList.PopClip(1);

    DrawData.BuildFromCommandList(SiblingList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 16);

    TEST_SECTION("Every batch is scissored to the region its own geometry was recorded under");
    for (const FUIDrawBatch& Batch : DrawData.GetBatches())
    {
        TEST_EXPECT(Batch.bIsClipped);
        TEST_EXPECT(Batch.ScissorRectangle == LeftRegion || Batch.ScissorRectangle == RightRegion);
    }

    TEST_SECTION("Closing a region no longer leaks the one beneath it onto a later sibling");

    FDrawCommandList NestedList;
    NestedList.PushClip(0, LeftRegion);
    NestedList.PushClip(0, FRectangle(IntVector2(0, 0), 40, 40));
    NestedList.AddBox(1, FRectangle(IntVector2(5, 5), 10, 10), FFloatColor::White);
    NestedList.PopClip(1);
    NestedList.PopClip(1);

    NestedList.PushClip(0, RightRegion);
    NestedList.AddBox(2, FRectangle(IntVector2(210, 10), 20, 20), FFloatColor::White);
    NestedList.PopClip(2);

    DrawData.BuildFromCommandList(NestedList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 8);

    TEST_SECTION("A nested region is still intersected with its parent when it is recorded");
    FDrawCommandList IntersectedList;
    IntersectedList.PushClip(0, FRectangle(IntVector2(0, 0), 100, 100));
    IntersectedList.PushClip(0, FRectangle(IntVector2(50, 50), 100, 100));
    IntersectedList.AddBox(1, FRectangle(IntVector2(60, 60), 10, 10), FFloatColor::White);
    IntersectedList.PopClip(1);
    IntersectedList.PopClip(1);

    DrawData.BuildFromCommandList(IntersectedList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT_EQ(DrawData.GetBatches()[0].ScissorRectangle, FRectangle(IntVector2(50, 50), 50, 50));

    TEST_END();
}

bool UIDrawDataRoundedBox_Test()
{
    TEST_BEGIN();

    // Counted and measured against the bare silhouette, which the fringe would both inflate and widen
    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    const FRectangle Bounds(IntVector2(0, 0), 100, 20);

    TEST_SECTION("A box with no radius still becomes the four-vertex quad");
    FDrawCommandList SquareList;
    SquareList.AddBox(0, Bounds, FFloatColor::White);
    DrawData.BuildFromCommandList(SquareList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 6);

    TEST_SECTION("A rounded box is a fan of one triangle per outline segment");
    FDrawCommandList RoundedList;
    RoundedList.AddBox(0, Bounds, FFloatColor::White, 3.0f);
    DrawData.BuildFromCommandList(RoundedList);

    const int32 OutlineCount = 4 * (4 + 1);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), OutlineCount + 1);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), OutlineCount * 3);
    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT_EQ(DrawData.GetBatches()[0].IndexCount, OutlineCount * 3);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Atlas == nullptr);

    TEST_SECTION("The fan turns around the middle of the bounds");
    TEST_EXPECT(DrawData.GetVertices()[0].Position == Vector2(50.0f, 10.0f));

    constexpr float EdgeTolerance = 0.01f;

    TEST_SECTION("Every vertex stays inside the bounds and no vertex sits in a corner");
    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT(Vertex.Position.X >= -EdgeTolerance && Vertex.Position.X <= 100.0f + EdgeTolerance);
        TEST_EXPECT(Vertex.Position.Y >= -EdgeTolerance && Vertex.Position.Y <= 20.0f + EdgeTolerance);

        const bool bIsOnVerticalEdge   = Vertex.Position.X <= EdgeTolerance || Vertex.Position.X >= 100.0f - EdgeTolerance;
        const bool bIsOnHorizontalEdge = Vertex.Position.Y <= EdgeTolerance || Vertex.Position.Y >= 20.0f - EdgeTolerance;
        TEST_EXPECT(!bIsOnVerticalEdge || !bIsOnHorizontalEdge);
    }

    TEST_SECTION("The outline reaches all four edges, so the shape still fills the bounds");
    bool bTouchesLeft   = false;
    bool bTouchesRight  = false;
    bool bTouchesTop    = false;
    bool bTouchesBottom = false;

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        bTouchesLeft   |= Vertex.Position.X <= EdgeTolerance;
        bTouchesRight  |= Vertex.Position.X >= 100.0f - EdgeTolerance;
        bTouchesTop    |= Vertex.Position.Y <= EdgeTolerance;
        bTouchesBottom |= Vertex.Position.Y >= 20.0f - EdgeTolerance;
    }

    TEST_EXPECT(bTouchesLeft && bTouchesRight && bTouchesTop && bTouchesBottom);

    TEST_SECTION("A radius past half the shorter side is clamped rather than turned inside out");
    FDrawCommandList OversizedList;
    OversizedList.AddBox(0, FRectangle(IntVector2(0, 0), 10, 10), FFloatColor::White, 50.0f);
    DrawData.BuildFromCommandList(OversizedList);

    const int32 ClampedOutlineCount = 4 * (7 + 1);
    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), ClampedOutlineCount + 1);

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT(Vertex.Position.X >= -EdgeTolerance && Vertex.Position.X <= 10.0f + EdgeTolerance);
        TEST_EXPECT(Vertex.Position.Y >= -EdgeTolerance && Vertex.Position.Y <= 10.0f + EdgeTolerance);
    }

    TEST_SECTION("A line keeps its square ends, since it shares the box path");
    FDrawCommandList LineList;
    LineList.AddLine(0, FRectangle(IntVector2(0, 0), 1, 16), FFloatColor::White);
    DrawData.BuildFromCommandList(LineList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 6);

    TEST_END();
}

bool UIDrawDataRoundedBottomBar_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;
    DrawData.SetAntiAliasingEnabled(false);

    const FRectangle Bounds(IntVector2(0, 0), 120, 28);

    constexpr float Radius    = 6.0f;
    constexpr float Thickness = 3.0f;
    constexpr float FadeWidth = 8.0f;
    constexpr float Tolerance = 0.01f;

    FDrawCommandList BarList;
    BarList.AddRoundedBottomBar(0, Bounds, FCornerRadii(Radius), Thickness, FFloatColor::White, FadeWidth);
    DrawData.BuildFromCommandList(BarList);

    TEST_SECTION("The band is a strip of column quads, so it carries two vertices and six indices per column");
    TEST_EXPECT(!DrawData.IsEmpty());
    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Atlas == nullptr);

    const int32 VertexCount = DrawData.GetVertices().Size();
    TEST_EXPECT_EQ(VertexCount % 2, 0);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), ((VertexCount / 2) - 1) * 6);
    TEST_EXPECT_EQ(DrawData.GetBatches()[0].IndexCount, DrawData.GetIndices().Size());

    TEST_SECTION("It sits against the bottom of the rectangle and never rises above the band's own height");
    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT(Vertex.Position.Y >= 28.0f - Thickness - Tolerance);
        TEST_EXPECT(Vertex.Position.Y <= 28.0f + Tolerance);
    }

    TEST_SECTION("The corner cuts it short of either edge, since the arc swallows the band before the rectangle ends");
    float MinX = 1000.0f;
    float MaxX = -1000.0f;

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        MinX = Math::Min(MinX, Vertex.Position.X);
        MaxX = Math::Max(MaxX, Vertex.Position.X);
    }

    TEST_EXPECT(MinX > 0.0f);
    TEST_EXPECT(MaxX < 120.0f);
    TEST_EXPECT(MinX < Radius);
    TEST_EXPECT(MaxX > 120.0f - Radius);

    TEST_SECTION("Each end tapers into the curve, so the first column is shorter than one in the middle");
    const int32 ColumnCount  = VertexCount / 2;
    const int32 MiddleColumn = ColumnCount / 2;

    const float FirstHeight  = DrawData.GetVertices()[1].Position.Y - DrawData.GetVertices()[0].Position.Y;
    const float LastHeight   = DrawData.GetVertices()[VertexCount - 1].Position.Y - DrawData.GetVertices()[VertexCount - 2].Position.Y;
    const float MiddleHeight = DrawData.GetVertices()[(MiddleColumn * 2) + 1].Position.Y - DrawData.GetVertices()[MiddleColumn * 2].Position.Y;

    TEST_EXPECT(FirstHeight < MiddleHeight);
    TEST_EXPECT(LastHeight < MiddleHeight);
    TEST_EXPECT(Math::Abs(MiddleHeight - Thickness) < Tolerance);

    TEST_SECTION("The alpha ramps from nothing at either end to full in the middle");
    TEST_EXPECT_EQ(DrawData.GetVertices()[0].Color >> 24, 0u);
    TEST_EXPECT_EQ(DrawData.GetVertices()[VertexCount - 1].Color >> 24, 0u);
    TEST_EXPECT_EQ(DrawData.GetVertices()[MiddleColumn * 2].Color >> 24, 255u);

    TEST_SECTION("Both vertices of one column share a colour, so the fade runs along the band and not across it");
    for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
    {
        TEST_EXPECT_EQ(DrawData.GetVertices()[ColumnIndex * 2].Color, DrawData.GetVertices()[(ColumnIndex * 2) + 1].Color);
    }

    TEST_SECTION("Asked for no fade, the band runs at full strength from end to end");
    FDrawCommandList SolidList;
    SolidList.AddRoundedBottomBar(0, Bounds, FCornerRadii(Radius), Thickness, FFloatColor::White, 0.0f);
    DrawData.BuildFromCommandList(SolidList);

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT_EQ(Vertex.Color >> 24, 255u);
    }

    TEST_SECTION("With square corners the band is a plain rectangle spanning the full width");
    FDrawCommandList SquareList;
    SquareList.AddRoundedBottomBar(0, Bounds, FCornerRadii(), Thickness, FFloatColor::White, 0.0f);
    DrawData.BuildFromCommandList(SquareList);

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        const bool bIsTop = Vertex.Position.Y <= 28.0f - Thickness + Tolerance;
        TEST_EXPECT(bIsTop || Vertex.Position.Y >= 28.0f - Tolerance);
        TEST_EXPECT(Vertex.Position.X >= -Tolerance && Vertex.Position.X <= 120.0f + Tolerance);
    }

    TEST_EXPECT(Math::Abs(DrawData.GetVertices()[0].Position.X) < Tolerance);
    TEST_EXPECT(Math::Abs(DrawData.GetVertices().Last().Position.X - 120.0f) < Tolerance);

    TEST_SECTION("A band with no thickness draws nothing at all");
    FDrawCommandList ThinList;
    ThinList.AddRoundedBottomBar(0, Bounds, FCornerRadii(Radius), 0.0f, FFloatColor::White, FadeWidth);
    DrawData.BuildFromCommandList(ThinList);

    TEST_EXPECT(DrawData.IsEmpty());

    TEST_END();
}

bool UIDrawDataText_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;

    TEST_SECTION("A metrics-only face lays out but draws nothing");
    TSharedPtr<FFixedWidthFontFace> FixedFont = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    FDrawCommandList FixedList;
    FixedList.AddText(0, FRectangle(IntVector2(0, 0), 100, 16), String("Hello"), FixedFont.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(FixedList);

    TEST_EXPECT(DrawData.IsEmpty());
    TEST_EXPECT(DrawData.GetVertices().IsEmpty());

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font)
    {
        TEST_END();
    }

    TEST_SECTION("Every printable glyph becomes one quad");
    FDrawCommandList TextList;
    TextList.AddText(0, FRectangle(IntVector2(0, 0), 100, 16), String("AB"), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(TextList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 8);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 12);
    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Atlas == Font->GetAtlas());

    TEST_SECTION("Whitespace advances the pen without adding a quad");
    FDrawCommandList SpacedList;
    SpacedList.AddText(0, FRectangle(IntVector2(0, 0), 100, 16), String("A B"), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(SpacedList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 8);
    TEST_EXPECT(DrawData.GetVertices()[4].Position.X > DrawData.GetVertices()[0].Position.X + Font->ShapeText(StringView("A")).Width);

    TEST_SECTION("The texture coordinates stay inside the atlas");
    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT(Vertex.TexCoord.X >= 0.0f && Vertex.TexCoord.X <= 1.0f);
        TEST_EXPECT(Vertex.TexCoord.Y >= 0.0f && Vertex.TexCoord.Y <= 1.0f);
    }

    TEST_SECTION("An empty string draws nothing");
    FDrawCommandList EmptyTextList;
    EmptyTextList.AddText(0, FRectangle(IntVector2(0, 0), 100, 16), String(), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(EmptyTextList);
    TEST_EXPECT(DrawData.IsEmpty());

    const int32 BandHeight = Font->GetTextBandHeight();
    TEST_EXPECT_EQ(BandHeight, Font->GetAscent() + Font->GetDescent());

    TEST_SECTION("A capital does not reach as far above the baseline as the face claims a glyph can");
    TEST_EXPECT(Font->GetCapHeight() < Font->GetAscent());

    TEST_SECTION("A rectangle taller than the glyphs need leaves a line of capitals the same space above and below");

    for (int32 Padding = 2; Padding <= 6; ++Padding)
    {
        const int32 BoxHeight = BandHeight + (Padding * 2);

        FDrawCommandList PaddedList;
        PaddedList.AddText(0, FRectangle(IntVector2(0, 0), 100, BoxHeight), String("HI"), Font.Get(), FFloatColor::White);
        DrawData.BuildFromCommandList(PaddedList);

        float InkTop    = static_cast<float>(BoxHeight);
        float InkBottom = 0.0f;
        for (const FUIVertex& Vertex : DrawData.GetVertices())
        {
            InkTop    = Math::Min(InkTop, Vertex.Position.Y);
            InkBottom = Math::Max(InkBottom, Vertex.Position.Y);
        }

        TEST_EXPECT(InkTop == static_cast<float>(BoxHeight) - InkBottom);
    }

    TEST_SECTION("A descender hangs into that room rather than the two cases splitting the difference");

    const int32 TallBoxHeight = BandHeight + 8;

    FDrawCommandList DescenderList;
    DescenderList.AddText(0, FRectangle(IntVector2(0, 0), 100, TallBoxHeight), String("Hg"), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(DescenderList);

    float DescenderTop    = static_cast<float>(TallBoxHeight);
    float DescenderBottom = 0.0f;
    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        DescenderTop    = Math::Min(DescenderTop, Vertex.Position.Y);
        DescenderBottom = Math::Max(DescenderBottom, Vertex.Position.Y);
    }

    TEST_EXPECT(DescenderBottom > static_cast<float>(TallBoxHeight) - DescenderTop);
    TEST_EXPECT(DescenderBottom <= static_cast<float>(TallBoxHeight));

    TEST_SECTION("A rectangle with no room to spare starts the band at the top rather than pushing glyphs out of it");

    FDrawCommandList TightList;
    TightList.AddText(0, FRectangle(IntVector2(0, 0), 100, BandHeight), String("A"), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(TightList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    const float TightTop = DrawData.GetVertices()[0].Position.Y;
    TEST_EXPECT(TightTop == static_cast<float>(Font->GetAscent() - Font->GetCapHeight()));

    FDrawCommandList ShortList;
    ShortList.AddText(0, FRectangle(IntVector2(0, 0), 100, 1), String("A"), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(ShortList);

    TEST_EXPECT(DrawData.GetVertices()[0].Position.Y == TightTop);

    TEST_SECTION("Text and boxes land in separate batches, since they sample different textures");
    FDrawCommandList MixedList;
    MixedList.AddBox(0, FRectangle(IntVector2(0, 0), 100, 16), FFloatColor::White);
    MixedList.AddText(1, FRectangle(IntVector2(0, 0), 100, 16), String("A"), Font.Get(), FFloatColor::White);
    MixedList.AddBox(2, FRectangle(IntVector2(0, 16), 100, 16), FFloatColor::White);
    DrawData.BuildFromCommandList(MixedList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 3);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Atlas == nullptr);
    TEST_EXPECT(DrawData.GetBatches()[1].Texture.Atlas == Font->GetAtlas());
    TEST_EXPECT(DrawData.GetBatches()[2].Texture.Atlas == nullptr);

    TEST_END();
}

bool UIDrawDataAntiAliasing_Test()
{
    TEST_BEGIN();

    constexpr float Tolerance  = 0.01f;
    constexpr float HalfFringe = FUIDrawData::FringeWidth * 0.5f;

    FUIDrawData DrawData;

    TEST_SECTION("It is on by default, and turning it off is what the counting tests rely on");
    TEST_EXPECT(DrawData.IsAntiAliasingEnabled());

    DrawData.SetAntiAliasingEnabled(false);
    TEST_EXPECT(!DrawData.IsAntiAliasingEnabled());

    DrawData.SetAntiAliasingEnabled(true);

    TEST_SECTION("A square box is still the plain quad, since an axis-aligned edge already rasterizes crisp");
    FDrawCommandList SquareList;
    SquareList.AddBox(0, FRectangle(IntVector2(0, 0), 100, 20), FFloatColor::White);
    DrawData.BuildFromCommandList(SquareList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 6);

    TEST_SECTION("A rounded box gains a second ring, so it is two vertices and nine indices per outline point");
    const FRectangle Bounds(IntVector2(0, 0), 100, 20);

    FDrawCommandList RoundedList;
    RoundedList.AddBox(0, Bounds, FFloatColor::White, 3.0f);
    DrawData.BuildFromCommandList(RoundedList);

    const int32 OutlineCount = 4 * (4 + 1);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), (OutlineCount * 2) + 1);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), OutlineCount * 9);
    TEST_EXPECT_EQ(DrawData.GetBatches()[0].IndexCount, OutlineCount * 9);

    TEST_SECTION("The inner ring carries the fill's alpha and the outer one carries none, which is what softens the edge");
    for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
    {
        const FUIVertex& Inner = DrawData.GetVertices()[1 + (OutlineIndex * 2)];
        const FUIVertex& Outer = DrawData.GetVertices()[2 + (OutlineIndex * 2)];

        TEST_EXPECT_EQ(Inner.Color >> 24, 0xffu);
        TEST_EXPECT_EQ(Outer.Color >> 24, 0x00u);

        const float Separation = (Outer.Position - Inner.Position).GetLength();
        TEST_EXPECT(Separation >= FUIDrawData::FringeWidth - Tolerance);
        TEST_EXPECT(Separation <= FUIDrawData::FringeWidth * FUIDrawData::MiterLimit);
    }

    TEST_SECTION("The rings straddle the silhouette, so the fringe is added rather than eaten out of the fill");
    float InnerMinX = Bounds.GetRight();
    float OuterMinX = Bounds.GetRight();

    for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
    {
        InnerMinX = Math::Min(InnerMinX, DrawData.GetVertices()[1 + (OutlineIndex * 2)].Position.X);
        OuterMinX = Math::Min(OuterMinX, DrawData.GetVertices()[2 + (OutlineIndex * 2)].Position.X);
    }

    TEST_EXPECT(Math::Abs(InnerMinX - HalfFringe) <= Tolerance);
    TEST_EXPECT(Math::Abs(OuterMinX + HalfFringe) <= Tolerance);

    TEST_SECTION("A stroke gains a fringe either side, so it is four vertices per point and eighteen indices per segment");
    FDrawCommandList LineList;
    LineList.AddLine(0, Vector2(0.0f, 10.0f), Vector2(100.0f, 10.0f), FFloatColor::White, 4.0f);
    DrawData.BuildFromCommandList(LineList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 2 * 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 18);

    TEST_SECTION("The clear rows are the outermost two and the opaque core sits between them at the nominal width");
    TEST_EXPECT_EQ(DrawData.GetVertices()[0].Color >> 24, 0x00u);
    TEST_EXPECT_EQ(DrawData.GetVertices()[1].Color >> 24, 0xffu);
    TEST_EXPECT_EQ(DrawData.GetVertices()[2].Color >> 24, 0xffu);
    TEST_EXPECT_EQ(DrawData.GetVertices()[3].Color >> 24, 0x00u);

    TEST_EXPECT(Math::Abs(DrawData.GetVertices()[0].Position.Y - (12.0f + HalfFringe)) <= Tolerance);
    TEST_EXPECT(Math::Abs(DrawData.GetVertices()[1].Position.Y - (12.0f - HalfFringe)) <= Tolerance);
    TEST_EXPECT(Math::Abs(DrawData.GetVertices()[3].Position.Y - (8.0f - HalfFringe)) <= Tolerance);

    TEST_SECTION("A stroke thinner than the fringe collapses its core and pays for the width in alpha instead");
    FDrawCommandList HairlineList;
    HairlineList.AddLine(0, Vector2(0.0f, 10.0f), Vector2(100.0f, 10.0f), FFloatColor::White, 0.5f);
    DrawData.BuildFromCommandList(HairlineList);

    TEST_EXPECT_EQ(DrawData.GetVertices()[1].Color >> 24, 0x80u);
    TEST_EXPECT_EQ(DrawData.GetVertices()[0].Color >> 24, 0x00u);
    TEST_EXPECT(Math::Abs(DrawData.GetVertices()[1].Position.Y - 10.0f) <= Tolerance);
    TEST_EXPECT(Math::Abs(DrawData.GetVertices()[2].Position.Y - 10.0f) <= Tolerance);
    TEST_EXPECT(Math::Abs(DrawData.GetVertices()[0].Position.Y - (10.0f + HalfFringe)) <= Tolerance);

    TEST_SECTION("A convex polygon keeps its fan and has the fringe stitched around the outside of it");
    const Vector2 Quad[4] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(10.0f, 0.0f),
        Vector2(10.0f, 10.0f),
        Vector2(0.0f, 10.0f),
    };

    FDrawCommandList QuadList;
    QuadList.AddConvexPolygon(0, TArrayView<const Vector2>(Quad, 4), FFloatColor::White);
    DrawData.BuildFromCommandList(QuadList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4 * 2);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), (2 * 3) + (4 * 6));

    TEST_EXPECT_EQ(DrawData.GetVertices()[0].Color >> 24, 0xffu);
    TEST_EXPECT_EQ(DrawData.GetVertices()[1].Color >> 24, 0x00u);

    TEST_SECTION("The accent bar hangs a clear row under the arc, which is the only edge of it that curves");
    const FRectangle BarBounds(IntVector2(0, 0), 120, 28);

    FDrawCommandList BarList;
    BarList.AddRoundedBottomBar(0, BarBounds, FCornerRadii(6.0f), 3.0f, FFloatColor::White, 0.0f);
    DrawData.BuildFromCommandList(BarList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size() % 3, 0);

    const int32 ColumnCount = (DrawData.GetVertices().Size() / 3) - 1;
    TEST_EXPECT(ColumnCount >= FUIDrawData::MinBarColumns);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), ColumnCount * 12);

    for (int32 ColumnIndex = 0; ColumnIndex <= ColumnCount; ++ColumnIndex)
    {
        const FUIVertex& Bottom = DrawData.GetVertices()[(ColumnIndex * 3) + 1];
        const FUIVertex& Fringe = DrawData.GetVertices()[(ColumnIndex * 3) + 2];

        TEST_EXPECT_EQ(Bottom.Color >> 24, 0xffu);
        TEST_EXPECT_EQ(Fringe.Color >> 24, 0x00u);
        TEST_EXPECT(Fringe.Position.Y > Bottom.Position.Y);
    }

    TEST_SECTION("Away from the corners it straddles the underside, so the band keeps the depth it was asked for");
    const FUIVertex& MiddleBottom = DrawData.GetVertices()[((ColumnCount / 2) * 3) + 1];
    const FUIVertex& MiddleFringe = DrawData.GetVertices()[((ColumnCount / 2) * 3) + 2];

    const float BarUnderside = static_cast<float>(BarBounds.GetBottom());
    TEST_EXPECT(Math::Abs(MiddleBottom.Position.Y - (BarUnderside - HalfFringe)) <= Tolerance);
    TEST_EXPECT(Math::Abs(MiddleFringe.Position.Y - (BarUnderside + HalfFringe)) <= Tolerance);

    TEST_SECTION("Turning it off gives back exactly the geometry the counting tests were written against");
    DrawData.SetAntiAliasingEnabled(false);
    DrawData.BuildFromCommandList(RoundedList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), OutlineCount + 1);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), OutlineCount * 3);

    TEST_END();
}
