#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/StringView.h"
#include "Core/Math/Math.h"

class FFontAtlas;
struct FGlyph;

struct FShapedGlyph
{
    FShapedGlyph()
        : Offset(0)
        , Advance(0)
        , SourceIndex(0)
        , Glyph(nullptr)
    {
    }

    /** @brief The pen position of this glyph's origin, relative to the start of the run. */
    int32 Offset;

    /** @brief How far the pen moves over this glyph, with the kerning against the one after it folded in. */
    int32 Advance;

    /** @brief The index in the source text this glyph came from. */
    int32 SourceIndex;

    /** @brief The glyph in the atlas, which is null for a face that has none to draw from. */
    const FGlyph* Glyph;
};

struct FShapedRun
{
    FShapedRun()
        : Glyphs()
        , Width(0)
    {
    }

    /** @brief The glyphs in the order they are drawn, one for every character of the source text. */
    TArray<FShapedGlyph> Glyphs;

    /** @brief How far the pen moved over the whole run, in pixels. */
    int32 Width;
};

struct IFontFace
{
    virtual ~IFontFace() = default;

    /**
     * @brief The rasterized glyphs backing this face, when it has any. A face that only answers metrics
     * returns null, which leaves it usable for layout and for tests while producing no text geometry.
     *
     * @return The atlas, or null when the face cannot be drawn.
     */
    virtual const FFontAtlas* GetAtlas() const = 0;

    /**
     * @brief Gets the distance from one baseline to the next.
     *
     * @return The distance in pixels, which carries the gap the face asks for between two lines and is
     * therefore not the height of the glyphs themselves.
     */
    virtual int32 GetLineHeight() const = 0;

    /** @return How far the tallest glyph reaches above the baseline, in pixels, as a positive value. */
    virtual int32 GetAscent() const = 0;

    /** @return How far the lowest glyph reaches below the baseline, in pixels, as a positive value. */
    virtual int32 GetDescent() const = 0;

    /**
     * @brief Gets how far a capital reaches above the baseline, which is what the eye reads as the top
     * of a line.
     *
     * @return The distance in pixels, which is never more than the ascent.
     */
    virtual int32 GetCapHeight() const = 0;

    /**
     * @brief Positions the glyphs of a run, applying the kerning between each pair. The one place a pen is
     * walked over text, so that measuring, hit-testing and drawing cannot disagree about where a glyph
     * sits, which they would the moment any of them summed per-character advances that kerning corrects.
     *
     * @param Text The text to shape.
     * @return The positioned glyphs, cached on the face so that asking twice in a frame costs a lookup.
     * The reference is good until the next call, and holding one across frames is not.
     */
    virtual const FShapedRun& ShapeText(const StringView& Text) const = 0;

    /**
     * @param Text The text to measure.
     * @return The width in pixels of the text laid out on a single line.
     */
    NODISCARD int32 MeasureWidth(const StringView& Text) const
    {
        return ShapeText(Text).Width;
    }

    /**
     * @brief Finds the character position closest to a horizontal offset, which is what maps a click to a position in the text.
     *
     * @param Text    The text to measure against.
     * @param OffsetX The offset from the start of the text, in pixels.
     * @return The index of the closest position in the text.
     */
    NODISCARD int32 FindCharacterIndexAtOffset(const StringView& Text, int32 OffsetX) const
    {
        if (OffsetX <= 0)
        {
            return 0;
        }

        for (const FShapedGlyph& Shaped : ShapeText(Text).Glyphs)
        {
            // Break at the half-glyph so a click on the right half of a character lands after it
            if (OffsetX < Shaped.Offset + (Shaped.Advance / 2))
            {
                return Shaped.SourceIndex;
            }
        }

        return Text.Length();
    }

    /**
     * @brief Gets where one character of a run sits and how much room it takes, which is what places a
     * caret on it.
     *
     * @param Text           The text the character belongs to.
     * @param CharacterIndex The index of the character, which may be one past the last for the caret that
     *                       sits at the end of the text.
     * @param OutOffset      Set to the distance from the start of the run to the character, in pixels.
     * @param OutAdvance     Set to how far the pen moves over the character, in pixels, and to zero for
     *                       the position past the end.
     */
    void GetCharacterPlacement(const StringView& Text, int32 CharacterIndex, int32& OutOffset, int32& OutAdvance) const
    {
        const FShapedRun& Run = ShapeText(Text);

        if (CharacterIndex >= Run.Glyphs.Size())
        {
            OutOffset  = Run.Width;
            OutAdvance = 0;
            return;
        }

        const int32 ClampedIndex = Math::Max(CharacterIndex, 0);

        OutOffset  = Run.Glyphs[ClampedIndex].Offset;
        OutAdvance = Run.Glyphs[ClampedIndex].Advance;
    }

    /**
     * @brief The height of the band the glyphs occupy, which is the ascent and the descent together.
     * Not the line height, which also carries the gap a face asks for between two lines: Consolas at
     * 16 pixels reports a 19 pixel line for a 17 pixel band, so centring the line box would leave the
     * glyphs sitting high inside it.
     *
     * @return The height of the band in pixels.
     */
    NODISCARD int32 GetTextBandHeight() const
    {
        return GetAscent() + GetDescent();
    }

    /**
     * @brief How far down that band starts inside a rectangle, so that the text looks centred in it.
     * The one place the centring is decided, so the glyphs, the caret and a selection fill cannot drift
     * apart. What gets centred is the capitals rather than the band, because the room a face reserves
     * below the baseline for descenders is empty in most of what a console shows: centring the band
     * itself leaves a line like "Console" with four more pixels under it than over it, which is what
     * reads as the text sitting high. Text that does have descenders hangs lower into that reserve,
     * the way it does in print and everywhere else, rather than the two cases fighting over the middle.
     * The band is still kept inside the rectangle, so a tight one degrades to sitting at the top rather
     * than pushing glyphs out through the bottom.
     *
     * @param AvailableHeight The height of the rectangle the text is drawn into.
     * @return The offset from the top of the rectangle in pixels, which is zero when there is no room.
     */
    NODISCARD int32 GetTextBandOffset(int32 AvailableHeight) const
    {
        const int32 BaselineY = (AvailableHeight + GetCapHeight()) / 2;
        return Math::Clamp(BaselineY - GetAscent(), 0, Math::Max(0, AvailableHeight - GetTextBandHeight()));
    }

    /**
     * @brief Gets how tall the text cursor is drawn, which is the capitals with the descent added above
     * and below.
     *
     * @param AvailableHeight The height of the rectangle the text is drawn into.
     * @return The height in pixels, cut down to the available height when it does not fit.
     */
    NODISCARD int32 GetTextCursorHeight(int32 AvailableHeight) const
    {
        return Math::Min(GetCapHeight() + (GetDescent() * 2), AvailableHeight);
    }

    /**
     * @brief Gets how far down the text cursor starts inside a rectangle. Placed off the same baseline
     * the glyphs are, so that it runs from one descent above the capitals to one descent below the
     * baseline and cannot drift away from the text it sits in.
     *
     * @param AvailableHeight The height of the rectangle the text is drawn into.
     * @return The offset from the top of the rectangle in pixels, clamped so the whole cursor stays
     * inside it.
     */
    NODISCARD int32 GetTextCursorOffset(int32 AvailableHeight) const
    {
        const int32 BaselineY = GetTextBandOffset(AvailableHeight) + GetAscent();
        return Math::Clamp(BaselineY - GetCapHeight() - GetDescent(), 0, Math::Max(0, AvailableHeight - GetTextCursorHeight(AvailableHeight)));
    }
};
