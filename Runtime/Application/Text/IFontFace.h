#pragma once
#include "Core/Containers/StringView.h"

struct IFontFace
{
    virtual ~IFontFace() = default;

    /** @brief Baseline to baseline distance in pixels. */
    virtual int32 GetLineHeight() const = 0;

    /** @brief Distance from the baseline up to the top of the tallest glyph, in pixels. */
    virtual int32 GetAscent() const = 0;

    /** @brief Distance from the baseline down to the lowest glyph, as a positive value. */
    virtual int32 GetDescent() const = 0;

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
};
