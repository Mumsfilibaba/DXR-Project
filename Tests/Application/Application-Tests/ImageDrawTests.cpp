#include "ImageDrawTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Misc/Paths.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/UIDrawData.h>
#include <Application/Text/TrueTypeFontFace.h>

// The tessellator only ever compares the pointer, so a texture never has to exist for these tests
static FRHITexture* MakeTextureKey(UPTR_INT Key)
{
    return reinterpret_cast<FRHITexture*>(Key);
}

/** @brief Whether any quad in the geometry covers exactly this rectangle. */
static bool HasQuadCovering(const FUIDrawData& DrawData, const FRectangle& Bounds)
{
    const TArray<FUIVertex>& Vertices = DrawData.GetVertices();

    for (int32 Index = 0; Index + 3 < Vertices.Size(); Index += 4)
    {
        const Vector2 TopLeft     = Vertices[Index].Position;
        const Vector2 BottomRight = Vertices[Index + 2].Position;

        if (TopLeft == Vector2(static_cast<float>(Bounds.Position.X), static_cast<float>(Bounds.Position.Y)) &&
            BottomRight == Vector2(static_cast<float>(Bounds.GetRight()), static_cast<float>(Bounds.GetBottom())))
        {
            return true;
        }
    }

    return false;
}

bool ImageBrush_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A default brush names no texture and samples the whole of it");
    const FUIBrush Empty;
    TEST_EXPECT(!Empty.IsValid());
    TEST_EXPECT(!Empty.IsNineSlice());
    TEST_EXPECT(Empty.MinTexCoord == Vector2(0.0f, 0.0f));
    TEST_EXPECT(Empty.MaxTexCoord == Vector2(1.0f, 1.0f));

    TEST_SECTION("A brush over a texture is valid and still samples the whole of it");
    const FUIBrush Whole(MakeTextureKey(0x10));
    TEST_EXPECT(Whole.IsValid());
    TEST_EXPECT(!Whole.IsNineSlice());
    TEST_EXPECT(Whole.Texture == MakeTextureKey(0x10));

    TEST_SECTION("A margin on any side turns the brush into a nine-slice");
    FUIBrush Sliced(MakeTextureKey(0x10));
    Sliced.Margin = FMargin(4, 0, 0, 0);
    TEST_EXPECT(Sliced.IsNineSlice());

    Sliced.Margin = FMargin(0, 0, 0, 4);
    TEST_EXPECT(Sliced.IsNineSlice());

    Sliced.Margin = FMargin();
    TEST_EXPECT(!Sliced.IsNineSlice());

    TEST_END();
}

bool ImageDrawQuad_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;

    const FRectangle Bounds(IntVector2(10, 20), 100, 40);

    TEST_SECTION("An image is one quad over its bounds");
    FDrawCommandList ImageList;
    ImageList.AddImage(0, Bounds, FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    DrawData.BuildFromCommandList(ImageList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 6);
    TEST_EXPECT(DrawData.GetVertices()[0].Position == Vector2(10.0f, 20.0f));
    TEST_EXPECT(DrawData.GetVertices()[2].Position == Vector2(110.0f, 60.0f));

    TEST_SECTION("It samples the whole texture by default");
    TEST_EXPECT(DrawData.GetVertices()[0].TexCoord == Vector2(0.0f, 0.0f));
    TEST_EXPECT(DrawData.GetVertices()[2].TexCoord == Vector2(1.0f, 1.0f));

    TEST_SECTION("A sub-region of an atlas page maps onto the same four corners");
    FUIBrush Region(MakeTextureKey(0x10));
    Region.MinTexCoord = Vector2(0.25f, 0.5f);
    Region.MaxTexCoord = Vector2(0.75f, 1.0f);

    FDrawCommandList RegionList;
    RegionList.AddImage(0, Bounds, Region, FFloatColor::White);
    DrawData.BuildFromCommandList(RegionList);

    TEST_EXPECT(DrawData.GetVertices()[0].TexCoord == Vector2(0.25f, 0.5f));
    TEST_EXPECT(DrawData.GetVertices()[1].TexCoord == Vector2(0.75f, 0.5f));
    TEST_EXPECT(DrawData.GetVertices()[2].TexCoord == Vector2(0.75f, 1.0f));
    TEST_EXPECT(DrawData.GetVertices()[3].TexCoord == Vector2(0.25f, 1.0f));

    TEST_SECTION("The tint reaches the vertices, so one white icon can be drawn in any color");
    FDrawCommandList TintedList;
    TintedList.AddImage(0, Bounds, FUIBrush(MakeTextureKey(0x10)), FFloatColor(1.0f, 0.0f, 0.0f, 1.0f));
    DrawData.BuildFromCommandList(TintedList);

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT_EQ(Vertex.Color, 0xff0000ffu);
    }

    TEST_SECTION("An image with no extent draws nothing");
    FDrawCommandList EmptyList;
    EmptyList.AddImage(0, FRectangle(IntVector2(0, 0), 0, 40), FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    DrawData.BuildFromCommandList(EmptyList);
    TEST_EXPECT(DrawData.IsEmpty());

    TEST_SECTION("One that misses the clip region is culled like any other command");
    FDrawCommandList CulledList;
    CulledList.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    CulledList.AddImage(1, FRectangle(IntVector2(400, 400), 40, 40), FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    CulledList.PopClip(2);

    DrawData.BuildFromCommandList(CulledList);
    TEST_EXPECT(DrawData.IsEmpty());

    TEST_END();
}

bool ImageNineSlice_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;

    FUIBrush Brush(MakeTextureKey(0x10));
    Brush.Margin = FMargin(10, 8, 10, 8);

    const FRectangle Bounds(IntVector2(0, 0), 100, 40);

    TEST_SECTION("A nine-slice is nine quads rather than one");
    FDrawCommandList SlicedList;
    SlicedList.AddImage(0, Bounds, Brush, FFloatColor::White);
    DrawData.BuildFromCommandList(SlicedList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 9 * 4);
    TEST_EXPECT_EQ(DrawData.GetIndices().Size(), 9 * 6);

    TEST_SECTION("The corners keep the pixel size the margin asked for");
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(0, 0), 10, 8)));
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(90, 0), 10, 8)));
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(0, 32), 10, 8)));
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(90, 32), 10, 8)));

    TEST_SECTION("The middle takes whatever is left over");
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(10, 8), 80, 24)));

    TEST_SECTION("Widening the panel widens only the middle and the two rules across it");
    FDrawCommandList WideList;
    WideList.AddImage(0, FRectangle(IntVector2(0, 0), 300, 40), Brush, FFloatColor::White);
    DrawData.BuildFromCommandList(WideList);

    TEST_EXPECT_EQ(DrawData.GetVertices().Size(), 9 * 4);
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(0, 0), 10, 8)));
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(290, 0), 10, 8)));
    TEST_EXPECT(HasQuadCovering(DrawData, FRectangle(IntVector2(10, 8), 280, 24)));

    TEST_SECTION("The source is cut at the same fractions the destination is");
    const TArray<FUIVertex>& Vertices = DrawData.GetVertices();

    bool bFoundTopLeftPatch = false;
    for (int32 Index = 0; Index + 3 < Vertices.Size(); Index += 4)
    {
        if (Vertices[Index].Position == Vector2(0.0f, 0.0f))
        {
            bFoundTopLeftPatch = true;
            TEST_EXPECT(Vertices[Index].TexCoord == Vector2(0.0f, 0.0f));
            TEST_EXPECT(Vertices[Index + 2].TexCoord == Vector2(10.0f / 300.0f, 8.0f / 40.0f));
        }
    }

    TEST_EXPECT(bFoundTopLeftPatch);

    TEST_SECTION("A panel narrower than its own border collapses the patches that no longer fit");
    FDrawCommandList NarrowList;
    NarrowList.AddImage(0, FRectangle(IntVector2(0, 0), 12, 40), Brush, FFloatColor::White);
    DrawData.BuildFromCommandList(NarrowList);

    TEST_EXPECT(DrawData.GetVertices().Size() < 9 * 4);

    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        TEST_EXPECT(Vertex.Position.X >= 0.0f && Vertex.Position.X <= 12.0f);
        TEST_EXPECT(Vertex.Position.Y >= 0.0f && Vertex.Position.Y <= 40.0f);
    }

    TEST_SECTION("Every patch stays in one batch, since they all sample the same texture");
    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Texture == MakeTextureKey(0x10));

    TEST_END();
}

bool ImageBatching_Test()
{
    TEST_BEGIN();

    FUIDrawData DrawData;

    const FRectangle Bounds(IntVector2(0, 0), 20, 20);

    TEST_SECTION("Two images off one texture share a batch");
    FDrawCommandList SameList;
    SameList.AddImage(0, Bounds, FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    SameList.AddImage(1, Bounds, FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    DrawData.BuildFromCommandList(SameList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT_EQ(DrawData.GetBatches()[0].IndexCount, 12);

    TEST_SECTION("Two different textures cannot");
    FDrawCommandList DifferentList;
    DifferentList.AddImage(0, Bounds, FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    DifferentList.AddImage(1, Bounds, FUIBrush(MakeTextureKey(0x20)), FFloatColor::White);
    DrawData.BuildFromCommandList(DifferentList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 2);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Texture == MakeTextureKey(0x10));
    TEST_EXPECT(DrawData.GetBatches()[1].Texture.Texture == MakeTextureKey(0x20));

    TEST_SECTION("A texture and a box are separate batches, and going back costs a third");
    FDrawCommandList MixedList;
    MixedList.AddBox(0, Bounds, FFloatColor::White);
    MixedList.AddImage(1, Bounds, FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    MixedList.AddBox(2, Bounds, FFloatColor::White);
    DrawData.BuildFromCommandList(MixedList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 3);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.IsEmpty());
    TEST_EXPECT(DrawData.GetBatches()[1].Texture.Texture == MakeTextureKey(0x10));
    TEST_EXPECT(DrawData.GetBatches()[2].Texture.IsEmpty());

    TEST_SECTION("An image over no texture is untextured geometry, so it joins the boxes");
    FDrawCommandList UntexturedList;
    UntexturedList.AddBox(0, Bounds, FFloatColor::White);
    UntexturedList.AddImage(1, Bounds, FUIBrush(), FFloatColor::White);
    DrawData.BuildFromCommandList(UntexturedList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 1);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.IsEmpty());

    TEST_SECTION("The batches still partition the index buffer without a gap");
    DrawData.BuildFromCommandList(MixedList);

    int32 ExpectedOffset = 0;
    for (const FUIDrawBatch& Batch : DrawData.GetBatches())
    {
        TEST_EXPECT_EQ(Batch.IndexOffset, ExpectedOffset);
        ExpectedOffset += Batch.IndexCount;
    }

    TEST_EXPECT_EQ(ExpectedOffset, DrawData.GetIndices().Size());

    TSharedPtr<FTrueTypeFontFace> Font = FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/consola.ttf", 16);
    TEST_EXPECT(Font != nullptr);

    if (!Font)
    {
        TEST_END();
    }

    TEST_SECTION("A glyph atlas and a texture are told apart even though both are textures");
    FDrawCommandList AtlasList;
    AtlasList.AddImage(0, Bounds, FUIBrush(MakeTextureKey(0x10)), FFloatColor::White);
    AtlasList.AddText(1, FRectangle(IntVector2(0, 0), 100, 16), String("A"), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(AtlasList);

    TEST_EXPECT_EQ(DrawData.GetBatches().Size(), 2);
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Texture == MakeTextureKey(0x10));
    TEST_EXPECT(DrawData.GetBatches()[0].Texture.Atlas == nullptr);
    TEST_EXPECT(DrawData.GetBatches()[1].Texture.Atlas == Font->GetAtlas());
    TEST_EXPECT(DrawData.GetBatches()[1].Texture.Texture == nullptr);

    TEST_END();
}
