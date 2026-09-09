#include "FontTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Misc/Paths.h>
#include <Application/Text/FontAtlas.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/TrueTypeFontFace.h>

static constexpr int32 GTestPixelHeight = 16;

static TSharedPtr<FTrueTypeFontFace> CreateTrueTypeFont(int32 PixelHeight = GTestPixelHeight)
{
    return FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/consola.ttf", PixelHeight);
}

static TSharedPtr<FTrueTypeFontFace> CreateProportionalFont(int32 PixelHeight = GTestPixelHeight)
{
    return FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/segoeui.ttf", PixelHeight);
}

bool FontAtlasPacking_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();

    TEST_SECTION("A font file rasterizes into a valid atlas");
    TEST_EXPECT(Font != nullptr);
    if (!Font)
    {
        TEST_END();
    }

    const FFontAtlas* Atlas = Font->GetAtlas();
    TEST_EXPECT(Atlas != nullptr);
    if (!Atlas)
    {
        TEST_END();
    }

    TEST_EXPECT(Atlas->IsValid());
    TEST_EXPECT(Atlas->GetWidth() > 0);
    TEST_EXPECT_EQ(Atlas->GetWidth(), Atlas->GetHeight());
    TEST_EXPECT(Atlas->GetPixels() != nullptr);

    TEST_SECTION("The pixels cover the whole atlas at four bytes each");
    const uint8* Pixels = Atlas->GetPixels();
    TEST_EXPECT(Pixels[0] == 255 && Pixels[1] == 255 && Pixels[2] == 255);

    TEST_SECTION("The vertical metrics describe a line taller than the ascent");
    TEST_EXPECT(Atlas->GetAscent() > 0);
    TEST_EXPECT(Atlas->GetDescent() >= 0);
    TEST_EXPECT(Atlas->GetLineHeight() >= Atlas->GetAscent() + Atlas->GetDescent());

    TEST_SECTION("The face reports the same metrics as its atlas");
    TEST_EXPECT_EQ(Font->GetAscent(), Atlas->GetAscent());
    TEST_EXPECT_EQ(Font->GetDescent(), Atlas->GetDescent());
    TEST_EXPECT_EQ(Font->GetLineHeight(), Atlas->GetLineHeight());

    TEST_SECTION("A larger size packs too, into an atlas at least as large");
    TSharedPtr<FTrueTypeFontFace> LargeFont = CreateTrueTypeFont(48);
    TEST_EXPECT(LargeFont != nullptr);

    if (LargeFont && LargeFont->GetAtlas())
    {
        TEST_EXPECT(LargeFont->GetAtlas()->GetWidth() >= Atlas->GetWidth());
        TEST_EXPECT(LargeFont->GetAscent() > Font->GetAscent());
    }

    TEST_SECTION("A build bumps the revision, so a renderer can spot a stale texture");
    TEST_EXPECT(Atlas->GetRevision() > 0);

    TEST_SECTION("An empty or zero-height build fails and leaves the atlas invalid");
    FFontAtlas EmptyAtlas;
    TEST_EXPECT(!EmptyAtlas.Build(TArray<uint8>(), GTestPixelHeight));
    TEST_EXPECT(!EmptyAtlas.IsValid());
    TEST_EXPECT_EQ(EmptyAtlas.GetRevision(), 0ull);

    TEST_SECTION("A metrics-only face has no atlas to sample");
    TSharedPtr<FFixedWidthFontFace> FixedFont = MakeSharedPtr<FFixedWidthFontFace>(8, 16);
    TEST_EXPECT(FixedFont->GetAtlas() == nullptr);

    TEST_END();
}

bool FontGlyphLookup_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font || !Font->GetAtlas())
    {
        TEST_END();
    }

    const FFontAtlas& Atlas = *Font->GetAtlas();

    TEST_SECTION("A printable glyph has geometry and an advance");
    const FGlyph& LetterA = Atlas.GetGlyph('A');
    TEST_EXPECT(!LetterA.AtlasRectangle.IsEmpty());
    TEST_EXPECT(LetterA.Advance > 0);

    TEST_SECTION("The glyph sits inside the atlas");
    TEST_EXPECT(LetterA.AtlasRectangle.Position.X >= 0);
    TEST_EXPECT(LetterA.AtlasRectangle.Position.Y >= 0);
    TEST_EXPECT(LetterA.AtlasRectangle.GetRight() <= Atlas.GetWidth());
    TEST_EXPECT(LetterA.AtlasRectangle.GetBottom() <= Atlas.GetHeight());

    TEST_SECTION("A glyph is lifted above the baseline");
    TEST_EXPECT(LetterA.BearingY < 0);

    TEST_SECTION("Space carries an advance but no geometry");
    const FGlyph& Space = Atlas.GetGlyph(' ');
    TEST_EXPECT(Space.AtlasRectangle.IsEmpty());
    TEST_EXPECT(Space.Advance > 0);

    TEST_SECTION("A character below the packed range falls back to space");
    const FGlyph& Tab = Atlas.GetGlyph('\t');
    TEST_EXPECT(Tab.AtlasRectangle.IsEmpty());
    TEST_EXPECT_EQ(Tab.Advance, Space.Advance);

    TEST_SECTION("A codepoint the face has no outline for falls back too");
    const FGlyph& Delete = Atlas.GetGlyph(127);
    TEST_EXPECT_EQ(Delete.Advance, Space.Advance);

    TEST_SECTION("A codepoint past the built range is rasterized when it is first asked for");

    const FGlyph LetterACopy        = LetterA;
    const uint64 RevisionBeforePage = Atlas.GetRevision();

    const FGlyph& Accented = Atlas.GetGlyph(0x00E9);
    TEST_EXPECT(!Accented.AtlasRectangle.IsEmpty());
    TEST_EXPECT(Accented.Advance > 0);
    TEST_EXPECT(Atlas.GetRevision() > RevisionBeforePage);

    TEST_SECTION("The page it arrived on stays, so asking again costs no revision");
    const uint64 RevisionAfterPage = Atlas.GetRevision();
    TEST_EXPECT(!Atlas.GetGlyph(0x00E8).AtlasRectangle.IsEmpty());
    TEST_EXPECT_EQ(Atlas.GetRevision(), RevisionAfterPage);

    TEST_SECTION("Growing for a page leaves the glyphs already packed intact");
    TEST_EXPECT(!Atlas.GetGlyph('A').AtlasRectangle.IsEmpty());
    TEST_EXPECT_EQ(Atlas.GetGlyph('A').Advance, LetterACopy.Advance);

    TEST_END();
}

bool FontKerning_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = CreateProportionalFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font || !Font->GetAtlas())
    {
        TEST_END();
    }

    const FFontAtlas& Atlas = *Font->GetAtlas();

    TEST_SECTION("A proportional face tucks a known pair together");
    const int32 Kerning = Atlas.GetKerning('A', 'V');
    TEST_EXPECT(Kerning < 0);

    TEST_SECTION("A pair with nothing to correct is left alone");
    TEST_EXPECT_EQ(Atlas.GetKerning('n', 'n'), 0);

    TEST_SECTION("The correction lands on the left glyph of the pair, so it moves the right one along");
    const FShapedRun& Shaped = Font->ShapeText(StringView("AV"));
    TEST_EXPECT_EQ(Shaped.Glyphs.Size(), 2);
    TEST_EXPECT_EQ(Shaped.Glyphs[1].Offset, Shaped.Glyphs[0].Advance);
    TEST_EXPECT_EQ(Shaped.Glyphs[0].Advance, Atlas.GetGlyph('A').Advance + Kerning);

    TEST_SECTION("The last glyph of a run has no pair to be corrected against");
    TEST_EXPECT_EQ(Shaped.Glyphs[1].Advance, Atlas.GetGlyph('V').Advance);

    TEST_SECTION("Measuring, hit testing and the caret all read the one shaping");
    const int32 KernedWidth = Font->MeasureWidth(StringView("AV"));
    TEST_EXPECT_EQ(KernedWidth, Atlas.GetGlyph('A').Advance + Atlas.GetGlyph('V').Advance + Kerning);

    int32 CaretOffset  = 0;
    int32 CaretAdvance = 0;
    Font->GetCharacterPlacement(StringView("AV"), 1, CaretOffset, CaretAdvance);
    TEST_EXPECT_EQ(CaretOffset, Atlas.GetGlyph('A').Advance + Kerning);

    TEST_SECTION("The caret past the last character sits at the end of the run");
    Font->GetCharacterPlacement(StringView("AV"), 2, CaretOffset, CaretAdvance);
    TEST_EXPECT_EQ(CaretOffset, KernedWidth);
    TEST_EXPECT_EQ(CaretAdvance, 0);

    TEST_END();
}

bool FontMeasurement_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font)
    {
        TEST_END();
    }

    TEST_SECTION("An empty string measures to nothing");
    TEST_EXPECT_EQ(Font->MeasureWidth(StringView("")), 0);

    TEST_SECTION("A string measures as the sum of the advances its shaping produced");
    int32 AdvanceA = 0;
    int32 AdvanceB = 0;
    int32 OffsetB  = 0;
    int32 WidthAB  = 0;
    {
        const FShapedRun& ShapedAB = Font->ShapeText(StringView("AB"));
        TEST_EXPECT_EQ(ShapedAB.Glyphs.Size(), 2);
        TEST_EXPECT_EQ(ShapedAB.Glyphs[0].Offset, 0);

        AdvanceA = ShapedAB.Glyphs[0].Advance;
        AdvanceB = ShapedAB.Glyphs[1].Advance;
        OffsetB  = ShapedAB.Glyphs[1].Offset;
        WidthAB  = ShapedAB.Width;
    }

    TEST_EXPECT_EQ(OffsetB, AdvanceA);
    TEST_EXPECT_EQ(WidthAB, AdvanceA + AdvanceB);
    TEST_EXPECT_EQ(Font->MeasureWidth(StringView("AB")), WidthAB);

    TEST_SECTION("A longer string is wider than a prefix of it");
    TEST_EXPECT(Font->MeasureWidth(StringView("Hello, World")) > Font->MeasureWidth(StringView("Hello")));

    TEST_SECTION("An offset before the text lands on the first character");
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(StringView("Hello"), -10), 0);
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(StringView("Hello"), 0), 0);

    TEST_SECTION("An offset past the text lands after the last character");
    const StringView Text("Hello");
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, Font->MeasureWidth(Text) + 100), Text.Length());

    TEST_SECTION("The left half of a character resolves to that character");
    const int32 FirstAdvance = Font->ShapeText(Text).Glyphs[0].Advance;
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, (FirstAdvance / 2) - 1), 0);

    TEST_SECTION("The right half resolves to the character after it");
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, FirstAdvance - 1), 1);

    TEST_END();
}
