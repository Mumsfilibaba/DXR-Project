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

    TEST_SECTION("A character above the packed range falls back too");
    const FGlyph& Delete = Atlas.GetGlyph(static_cast<CHAR>(127));
    TEST_EXPECT_EQ(Delete.Advance, Space.Advance);

    const FGlyph& HighByte = Atlas.GetGlyph(static_cast<CHAR>(200));
    TEST_EXPECT_EQ(HighByte.Advance, Space.Advance);

    TEST_SECTION("The face answers the same advance as the atlas");
    TEST_EXPECT_EQ(Font->GetCharacterAdvance('A'), LetterA.Advance);

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

    TEST_SECTION("A string measures as the sum of its advances");
    const int32 WidthA  = Font->GetCharacterAdvance('A');
    const int32 WidthB  = Font->GetCharacterAdvance('B');
    const int32 WidthAB = Font->MeasureWidth(StringView("AB"));
    TEST_EXPECT_EQ(WidthAB, WidthA + WidthB);

    TEST_SECTION("A longer string is wider than a prefix of it");
    TEST_EXPECT(Font->MeasureWidth(StringView("Hello, World")) > Font->MeasureWidth(StringView("Hello")));

    TEST_SECTION("An offset before the text lands on the first character");
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(StringView("Hello"), -10), 0);
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(StringView("Hello"), 0), 0);

    TEST_SECTION("An offset past the text lands after the last character");
    const StringView Text("Hello");
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, Font->MeasureWidth(Text) + 100), Text.Length());

    TEST_SECTION("The left half of a character resolves to that character");
    const int32 FirstAdvance = Font->GetCharacterAdvance('H');
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, (FirstAdvance / 2) - 1), 0);

    TEST_SECTION("The right half resolves to the character after it");
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, FirstAdvance - 1), 1);

    TEST_END();
}
