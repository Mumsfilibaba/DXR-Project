#pragma once
#include "Core/Containers/StringView.h"
#include "Core/Math/Math.h"

class FFontAtlas;

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

    /** @brief Baseline to baseline distance in pixels. */
    virtual int32 GetLineHeight() const = 0;

    /** @brief Distance from the baseline up to the top of the tallest glyph, in pixels. */
    virtual int32 GetAscent() const = 0;

    /** @brief Distance from the baseline down to the lowest glyph, as a positive value. */
    virtual int32 GetDescent() const = 0;

    /** @brief Distance from the baseline up to the top of a capital, which is what the eye reads as the top of a line. */
    virtual int32 GetCapHeight() const = 0;

    /**
     * @brief Horizontal advance of a single character in pixels.
     *
     * @param Character The character to measure.
     * @return The advance in pixels.
     */
    virtual int32 GetCharacterAdvance(CHAR Character) const = 0;

    /**
     * @brief Width in pixels of the text laid out on a single line.
     *
     * @param Text The text to measure.
     * @return The width in pixels.
     */
    virtual int32 MeasureWidth(const StringView& Text) const = 0;

    /**
     * @brief Finds the character position closest to a horizontal offset, which is what maps a click to a position in the text.
     *
     * @param Text    The text to measure against.
     * @param OffsetX The offset from the start of the text, in pixels.
     * @return The index of the closest position in the text.
     */
    virtual int32 FindCharacterIndexAtOffset(const StringView& Text, int32 OffsetX) const = 0;

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

    /** @brief The height of the text cursor, which is the capitals with the descent added above and below. */
    NODISCARD int32 GetTextCursorHeight(int32 AvailableHeight) const
    {
        return Math::Min(GetCapHeight() + (GetDescent() * 2), AvailableHeight);
    }

    /** @brief How far down the text cursor starts inside a rectangle, which centres it. */
    NODISCARD int32 GetTextCursorOffset(int32 AvailableHeight) const
    {
        const int32 BaselineY = GetTextBandOffset(AvailableHeight) + GetAscent();
        return Math::Clamp(BaselineY - GetCapHeight() - GetDescent(), 0, Math::Max(0, AvailableHeight - GetTextCursorHeight(AvailableHeight)));
    }
};
