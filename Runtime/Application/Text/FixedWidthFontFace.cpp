#include "Application/Text/FixedWidthFontFace.h"
#include "Core/Math/Math.h"

FFixedWidthFontFace::FFixedWidthFontFace(int32 InCharacterAdvance, int32 InLineHeight)
    : CharacterAdvance(Math::Max(1, InCharacterAdvance))
    , LineHeight(Math::Max(1, InLineHeight))
{
}

FFixedWidthFontFace::~FFixedWidthFontFace() = default;

int32 FFixedWidthFontFace::GetLineHeight() const
{
    return LineHeight;
}

int32 FFixedWidthFontFace::GetAscent() const
{
    // A monospaced face has no real metrics, so the baseline is placed at three quarters of the line
    return (LineHeight * 3) / 4;
}

int32 FFixedWidthFontFace::GetDescent() const
{
    return LineHeight - GetAscent();
}

int32 FFixedWidthFontFace::GetCharacterAdvance(CHAR /*Character*/) const
{
    return CharacterAdvance;
}

int32 FFixedWidthFontFace::MeasureWidth(const StringView& Text) const
{
    return Text.Length() * CharacterAdvance;
}

int32 FFixedWidthFontFace::FindCharacterIndexAtOffset(const StringView& Text, int32 OffsetX) const
{
    if (OffsetX <= 0)
    {
        return 0;
    }

    // Round to the nearest boundary so a click on the right half of a glyph lands after it
    const int32 Index = (OffsetX + (CharacterAdvance / 2)) / CharacterAdvance;
    return Math::Clamp(Index, 0, Text.Length());
}
