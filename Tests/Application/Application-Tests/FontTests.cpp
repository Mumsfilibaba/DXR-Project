#include "FontTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/StringConversion.h>
#include <Core/Misc/Paths.h>
#include <Application/Text/FontAtlas.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/TrueTypeFontFace.h>

static constexpr int32 GTestPixelHeight = 16;

static TSharedPtr<FTrueTypeFontFace> CreateTrueTypeFont(int32 PixelHeight = GTestPixelHeight)
{
    return FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + TEXT("/Editor/Fonts/consola.ttf"), PixelHeight);
}

static TSharedPtr<FTrueTypeFontFace> CreateProportionalFont(int32 PixelHeight = GTestPixelHeight)
{
    return FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + TEXT("/Editor/Fonts/segoeui.ttf"), PixelHeight);
}

static TArray<int32> FindCharacterStarts(const FString& Text)
{
    TArray<int32> Starts;
    for (int32 Index = 0; Index < Text.Length(); Index = FStringConversion::FindNextCodePoint(Text.Data(), Text.Length(), Index))
    {
        Starts.Add(Index);
    }

    return Starts;
}

static bool HaveSameCoverage(const FFontAtlas& Atlas, const FGlyph& First, const FGlyph& Second)
{
    if (First.AtlasRectangle.Width != Second.AtlasRectangle.Width || First.AtlasRectangle.Height != Second.AtlasRectangle.Height)
    {
        return false;
    }

    for (int32 Row = 0; Row < First.AtlasRectangle.Height; ++Row)
    {
        for (int32 Column = 0; Column < First.AtlasRectangle.Width; ++Column)
        {
            const int32 FirstTexel  = ((First.AtlasRectangle.Position.Y + Row) * Atlas.GetWidth()) + First.AtlasRectangle.Position.X + Column;
            const int32 SecondTexel = ((Second.AtlasRectangle.Position.Y + Row) * Atlas.GetWidth()) + Second.AtlasRectangle.Position.X + Column;
            if (Atlas.GetPixels()[(FirstTexel * 4) + 3] != Atlas.GetPixels()[(SecondTexel * 4) + 3])
            {
                return false;
            }
        }
    }

    return true;
}

bool FontAtlasPacking_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();

    TEST_SECTION(TEXT("A font file rasterizes into a valid atlas"));
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

    TEST_SECTION(TEXT("The pixels cover the whole atlas at four bytes each"));
    const uint8* Pixels = Atlas->GetPixels();
    TEST_EXPECT(Pixels[0] == 255 && Pixels[1] == 255 && Pixels[2] == 255);

    TEST_SECTION(TEXT("The vertical metrics describe a line taller than the ascent"));
    TEST_EXPECT(Atlas->GetAscent() > 0);
    TEST_EXPECT(Atlas->GetDescent() >= 0);
    TEST_EXPECT(Atlas->GetLineHeight() >= Atlas->GetAscent() + Atlas->GetDescent());

    TEST_SECTION(TEXT("The face reports the same metrics as its atlas"));
    TEST_EXPECT_EQ(Font->GetAscent(), Atlas->GetAscent());
    TEST_EXPECT_EQ(Font->GetDescent(), Atlas->GetDescent());
    TEST_EXPECT_EQ(Font->GetLineHeight(), Atlas->GetLineHeight());

    TEST_SECTION(TEXT("A larger size packs too, into an atlas at least as large"));
    TSharedPtr<FTrueTypeFontFace> LargeFont = CreateTrueTypeFont(48);
    TEST_EXPECT(LargeFont != nullptr);

    if (LargeFont && LargeFont->GetAtlas())
    {
        TEST_EXPECT(LargeFont->GetAtlas()->GetWidth() >= Atlas->GetWidth());
        TEST_EXPECT(LargeFont->GetAscent() > Font->GetAscent());
    }

    TEST_SECTION(TEXT("A build bumps the revision, so a renderer can spot a stale texture"));
    TEST_EXPECT(Atlas->GetRevision() > 0);

    TEST_SECTION(TEXT("An empty or zero-height build fails and leaves the atlas invalid"));
    FFontAtlas EmptyAtlas;
    TEST_EXPECT(!EmptyAtlas.Build(TArray<uint8>(), GTestPixelHeight));
    TEST_EXPECT(!EmptyAtlas.IsValid());
    TEST_EXPECT_EQ(EmptyAtlas.GetRevision(), 0ull);

    TEST_SECTION(TEXT("A metrics-only face has no atlas to sample"));
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

    TEST_SECTION(TEXT("A printable glyph has geometry and an advance"));
    const FGlyph& LetterA = Atlas.GetGlyph('A');
    TEST_EXPECT(!LetterA.AtlasRectangle.IsEmpty());
    TEST_EXPECT(LetterA.Advance > 0);

    TEST_SECTION(TEXT("The glyph sits inside the atlas"));
    TEST_EXPECT(LetterA.AtlasRectangle.Position.X >= 0);
    TEST_EXPECT(LetterA.AtlasRectangle.Position.Y >= 0);
    TEST_EXPECT(LetterA.AtlasRectangle.GetRight() <= Atlas.GetWidth());
    TEST_EXPECT(LetterA.AtlasRectangle.GetBottom() <= Atlas.GetHeight());

    TEST_SECTION(TEXT("A glyph is lifted above the baseline"));
    TEST_EXPECT(LetterA.BearingY < 0);

    TEST_SECTION(TEXT("Space carries an advance but no geometry"));
    const FGlyph& Space = Atlas.GetGlyph(' ');
    TEST_EXPECT(Space.AtlasRectangle.IsEmpty());
    TEST_EXPECT(Space.Advance > 0);

    TEST_SECTION(TEXT("A character below the packed range falls back to space"));
    const FGlyph& Tab = Atlas.GetGlyph('\t');
    TEST_EXPECT(Tab.AtlasRectangle.IsEmpty());
    TEST_EXPECT_EQ(Tab.Advance, Space.Advance);

    TEST_SECTION(TEXT("A codepoint the face has no outline for falls back too"));
    const FGlyph& Delete = Atlas.GetGlyph(127);
    TEST_EXPECT_EQ(Delete.Advance, Space.Advance);

    TEST_SECTION(TEXT("A codepoint past the built range is rasterized when it is first asked for"));

    const FGlyph LetterACopy        = LetterA;
    const uint64 RevisionBeforePage = Atlas.GetRevision();

    const FGlyph& Accented = Atlas.GetGlyph(0x00E9);
    TEST_EXPECT(!Accented.AtlasRectangle.IsEmpty());
    TEST_EXPECT(Accented.Advance > 0);
    TEST_EXPECT(Atlas.GetRevision() > RevisionBeforePage);

    TEST_SECTION(TEXT("The page it arrived on stays, so asking again costs no revision"));
    const uint64 RevisionAfterPage = Atlas.GetRevision();
    TEST_EXPECT(!Atlas.GetGlyph(0x00E8).AtlasRectangle.IsEmpty());
    TEST_EXPECT_EQ(Atlas.GetRevision(), RevisionAfterPage);

    TEST_SECTION(TEXT("Growing for a page leaves the glyphs already packed intact"));
    TEST_EXPECT(!Atlas.GetGlyph('A').AtlasRectangle.IsEmpty());
    TEST_EXPECT_EQ(Atlas.GetGlyph('A').Advance, LetterACopy.Advance);

    TEST_END();
}

bool FontAtlasIncremental_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font || !Font->GetAtlas())
    {
        TEST_END();
    }

    const FFontAtlas& Atlas = *Font->GetAtlas();

    TEST_SECTION(TEXT("A fresh atlas starts a layout with nothing changed since"));
    TEST_EXPECT_EQ(Atlas.GetLayoutRevision(), Atlas.GetRevision());
    TEST_EXPECT(Atlas.GetChangedSinceLayout().IsEmpty());

    TEST_SECTION(TEXT("A page that fits changes only its own texels; one that does not starts a new layout"));
    static const int32 PageSamples[] = { 0x00E9, 0x03B1, 0x0416, 0x2192, 0x2500 };

    bool bSawFit = false;
    for (int32 Codepoint : PageSamples)
    {
        const uint64      RevisionBefore = Atlas.GetRevision();
        const uint64      LayoutBefore   = Atlas.GetLayoutRevision();
        const FGlyph*     AsciiBefore    = &Atlas.GetGlyph('A');
        const FRectangle  AsciiRectangle = AsciiBefore->AtlasRectangle;

        const FGlyph& Glyph = Atlas.GetGlyph(Codepoint);
        TEST_EXPECT(Atlas.GetRevision() > RevisionBefore);

        if (Atlas.GetLayoutRevision() == LayoutBefore)
        {
            bSawFit = true;

            TEST_EXPECT(&Atlas.GetGlyph('A') == AsciiBefore);
            TEST_EXPECT(Atlas.GetGlyph('A').AtlasRectangle == AsciiRectangle);

            if (!Glyph.AtlasRectangle.IsEmpty())
            {
                TEST_EXPECT(Atlas.GetChangedSinceLayout().Union(Glyph.AtlasRectangle) == Atlas.GetChangedSinceLayout());
            }
        }
        else
        {
            TEST_EXPECT_EQ(Atlas.GetLayoutRevision(), Atlas.GetRevision());
            TEST_EXPECT(Atlas.GetChangedSinceLayout().IsEmpty());
        }
    }

    TEST_EXPECT(bSawFit);

    TEST_SECTION(TEXT("Asking again for a page already packed changes nothing"));
    const uint64 SettledRevision = Atlas.GetRevision();
    static_cast<void>(Atlas.GetGlyph(0x00E9));
    TEST_EXPECT_EQ(Atlas.GetRevision(), SettledRevision);

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

    TEST_SECTION(TEXT("A proportional face tucks a known pair together"));
    const int32 Kerning = Atlas.GetKerning('A', 'V');
    TEST_EXPECT(Kerning < 0);

    TEST_SECTION(TEXT("A pair with nothing to correct is left alone"));
    TEST_EXPECT_EQ(Atlas.GetKerning('n', 'n'), 0);

    TEST_SECTION(TEXT("The correction lands on the left glyph of the pair, so it moves the right one along"));
    const FShapedRun& Shaped = Font->ShapeText(FStringView(TEXT("AV")));
    TEST_EXPECT_EQ(Shaped.Glyphs.Size(), 2);
    TEST_EXPECT_EQ(Shaped.Glyphs[1].Offset, Shaped.Glyphs[0].Advance);
    TEST_EXPECT_EQ(Shaped.Glyphs[0].Advance, Atlas.GetGlyph('A').Advance + Kerning);

    TEST_SECTION(TEXT("The last glyph of a run has no pair to be corrected against"));
    TEST_EXPECT_EQ(Shaped.Glyphs[1].Advance, Atlas.GetGlyph('V').Advance);

    TEST_SECTION(TEXT("Measuring, hit testing and the caret all read the one shaping"));
    const int32 KernedWidth = Font->MeasureWidth(FStringView(TEXT("AV")));
    TEST_EXPECT_EQ(KernedWidth, Atlas.GetGlyph('A').Advance + Atlas.GetGlyph('V').Advance + Kerning);

    int32 CaretOffset  = 0;
    int32 CaretAdvance = 0;
    Font->GetCharacterPlacement(FStringView(TEXT("AV")), 1, CaretOffset, CaretAdvance);
    TEST_EXPECT_EQ(CaretOffset, Atlas.GetGlyph('A').Advance + Kerning);

    TEST_SECTION(TEXT("The caret past the last character sits at the end of the run"));
    Font->GetCharacterPlacement(FStringView(TEXT("AV")), 2, CaretOffset, CaretAdvance);
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

    TEST_SECTION(TEXT("An empty string measures to nothing"));
    TEST_EXPECT_EQ(Font->MeasureWidth(FStringView(TEXT(""))), 0);

    TEST_SECTION(TEXT("A string measures as the sum of the advances its shaping produced"));
    int32 AdvanceA = 0;
    int32 AdvanceB = 0;
    int32 OffsetB  = 0;
    int32 WidthAB  = 0;
    {
        const FShapedRun& ShapedAB = Font->ShapeText(FStringView(TEXT("AB")));
        TEST_EXPECT_EQ(ShapedAB.Glyphs.Size(), 2);
        TEST_EXPECT_EQ(ShapedAB.Glyphs[0].Offset, 0);

        AdvanceA = ShapedAB.Glyphs[0].Advance;
        AdvanceB = ShapedAB.Glyphs[1].Advance;
        OffsetB  = ShapedAB.Glyphs[1].Offset;
        WidthAB  = ShapedAB.Width;
    }

    TEST_EXPECT_EQ(OffsetB, AdvanceA);
    TEST_EXPECT_EQ(WidthAB, AdvanceA + AdvanceB);
    TEST_EXPECT_EQ(Font->MeasureWidth(FStringView(TEXT("AB"))), WidthAB);

    TEST_SECTION(TEXT("A longer string is wider than a prefix of it"));
    TEST_EXPECT(Font->MeasureWidth(FStringView(TEXT("Hello, World"))) > Font->MeasureWidth(FStringView(TEXT("Hello"))));

    TEST_SECTION(TEXT("An offset before the text lands on the first character"));
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(FStringView(TEXT("Hello")), -10), 0);
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(FStringView(TEXT("Hello")), 0), 0);

    TEST_SECTION(TEXT("An offset past the text lands after the last character"));
    const FStringView Text(TEXT("Hello"));
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, Font->MeasureWidth(Text) + 100), Text.Length());

    TEST_SECTION(TEXT("The left half of a character resolves to that character"));
    const int32 FirstAdvance = Font->ShapeText(Text).Glyphs[0].Advance;
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, (FirstAdvance / 2) - 1), 0);

    TEST_SECTION(TEXT("The right half resolves to the character after it"));
    TEST_EXPECT_EQ(Font->FindCharacterIndexAtOffset(Text, FirstAdvance - 1), 1);

    TEST_END();
}

bool FontElision_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = CreateTrueTypeFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font)
    {
        TEST_END();
    }

    const FStringView Text(TEXT("Ambient occlusion"));

    const int32 FullWidth     = Font->MeasureWidth(Text);
    const int32 EllipsisWidth = Font->MeasureWidth(FStringView(IFontFace::Ellipsis));

    TEST_SECTION(TEXT("Text that already fits comes back untouched, whatever room is left over"));
    TEST_EXPECT_EQ(Font->ElideText(Text, FullWidth), FString(TEXT("Ambient occlusion")));
    TEST_EXPECT_EQ(Font->ElideText(Text, FullWidth + 100), FString(TEXT("Ambient occlusion")));

    TEST_SECTION(TEXT("Text that does not fit is cut down and ends in the ellipsis"));
    const int32   Budget = FullWidth / 2;
    const FString Elided = Font->ElideText(Text, Budget);

    TEST_EXPECT(Elided.Length() < Text.Length());
    TEST_EXPECT(Elided.EndsWith(IFontFace::Ellipsis));

    TEST_SECTION(TEXT("What comes back stays inside the width it was given"));
    TEST_EXPECT(Font->MeasureWidth(FStringView(Elided.Data(), Elided.Length())) <= Budget);

    TEST_SECTION(TEXT("A width that cannot hold even the ellipsis comes back empty rather than overrunning"));
    TEST_EXPECT(Font->ElideText(Text, EllipsisWidth).IsEmpty());
    TEST_EXPECT(Font->ElideText(Text, 0).IsEmpty());
    TEST_EXPECT(Font->ElideText(Text, -10).IsEmpty());

    TEST_END();
}

bool FontUnicodeShaping_Test()
{
    TEST_BEGIN();

    const FString       Text = TEXT("Åäö 日本語 😀");
    const FStringView   TextView(Text.Data(), Text.Length());
    const TArray<int32> Starts = FindCharacterStarts(Text);

    const int32 ExpectedCodepoints[] = { 0xC5, 0xE4, 0xF6, ' ', 0x65E5, 0x672C, 0x8A9E, ' ', 0x1F600 };

    TSharedPtr<FTrueTypeFontFace> Font = CreateProportionalFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font)
    {
        TEST_END();
    }

    TSharedPtr<FFixedWidthFontFace> FixedFont = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TEST_SECTION(TEXT("A run has one glyph per character, however many code units each takes"));
    TEST_EXPECT_EQ(Starts.Size(), 9);

    for (const IFontFace* Face : { static_cast<const IFontFace*>(Font.Get()), static_cast<const IFontFace*>(FixedFont.Get()) })
    {
        const FShapedRun& Run = Face->ShapeText(TextView);
        TEST_EXPECT_EQ(Run.Glyphs.Size(), Starts.Size());

        if (Run.Glyphs.Size() == Starts.Size())
        {
            for (int32 Index = 0; Index < Starts.Size(); ++Index)
            {
                TEST_EXPECT_EQ(Run.Glyphs[Index].SourceIndex, Starts[Index]);
                TEST_EXPECT_EQ(Run.Glyphs[Index].Codepoint, ExpectedCodepoints[Index]);
            }
        }
    }

    TEST_EXPECT_EQ(FixedFont->MeasureWidth(TextView), 9 * 8);

    TEST_SECTION(TEXT("A caret anywhere inside a character sits where the character starts"));
    const FShapedRun& Run = Font->ShapeText(TextView);
    TArray<int32> GlyphOffsets;
    for (const FShapedGlyph& Glyph : Run.Glyphs)
    {
        GlyphOffsets.Add(Glyph.Offset);
    }

    for (int32 Character = 0; Character < Starts.Size(); ++Character)
    {
        const int32 End = (Character + 1 < Starts.Size()) ? Starts[Character + 1] : Text.Length();
        for (int32 Index = Starts[Character]; Index < End; ++Index)
        {
            int32 Offset  = -1;
            int32 Advance = -1;
            Font->GetCharacterPlacement(TextView, Index, Offset, Advance);
            TEST_EXPECT_EQ(Offset, GlyphOffsets[Character]);
        }
    }

    TEST_SECTION(TEXT("A click resolves to the start of a character, never into the middle of one"));
    const int32 TextWidth = Font->MeasureWidth(TextView);
    for (int32 OffsetX = -2; OffsetX <= TextWidth + 2; ++OffsetX)
    {
        const int32 Index = Font->FindCharacterIndexAtOffset(TextView, OffsetX);
        TEST_EXPECT(Starts.Contains(Index) || Index == Text.Length());
    }

    TEST_SECTION(TEXT("Elision cuts between characters, never through one"));
    const int32 EllipsisLength = FStringView(IFontFace::Ellipsis).Length();
    for (int32 MaxWidth = 1; MaxWidth < TextWidth; ++MaxWidth)
    {
        const FString Elided = Font->ElideText(TextView, MaxWidth);
        if (!Elided.IsEmpty())
        {
            TEST_EXPECT(Elided.EndsWith(IFontFace::Ellipsis));
            TEST_EXPECT(Starts.Contains(Elided.Length() - EllipsisLength));
        }
    }

    TEST_END();
}

bool FontFallback_Test()
{
    TEST_BEGIN();

#if PLATFORM_WINDOWS
    TSharedPtr<FTrueTypeFontFace> Font = CreateProportionalFont();
    TEST_EXPECT(Font != nullptr);
    if (!Font || !Font->GetAtlas())
    {
        TEST_END();
    }

    const FFontAtlas& Atlas = *Font->GetAtlas();

    const int32 Codepoints[] = { 0x65E5, 0x672C, 0x0378, 0x0379, 'a' };
    for (int32 Codepoint : Codepoints)
    {
        (void)Atlas.GetGlyph(Codepoint);
    }

    const FGlyph Sun      = Atlas.GetGlyph(0x65E5);
    const FGlyph Book     = Atlas.GetGlyph(0x672C);
    const FGlyph Missing  = Atlas.GetGlyph(0x0378);
    const FGlyph Missing2 = Atlas.GetGlyph(0x0379);
    const FGlyph LetterA  = Atlas.GetGlyph('a');

    TEST_SECTION(TEXT("Ideographs the editor's font lacks are drawn from a font installed with Windows"));
    TEST_EXPECT(!Sun.AtlasRectangle.IsEmpty());
    TEST_EXPECT(!Book.AtlasRectangle.IsEmpty());
    TEST_EXPECT(!HaveSameCoverage(Atlas, Sun, Book));

    TEST_SECTION(TEXT("They are sized to the same em, which makes them wider than a Latin letter but no wider than the line is tall"));
    TEST_EXPECT(Sun.Advance > LetterA.Advance);
    TEST_EXPECT(Sun.Advance <= Font->GetLineHeight());
    TEST_EXPECT(Sun.AtlasRectangle.Height <= Font->GetAscent() + Font->GetDescent());

    TEST_SECTION(TEXT("A code point that no font has draws the editor font's missing-glyph box"));
    TEST_EXPECT(!Missing.AtlasRectangle.IsEmpty());
    TEST_EXPECT(HaveSameCoverage(Atlas, Missing, Missing2));
#endif

    TEST_END();
}
