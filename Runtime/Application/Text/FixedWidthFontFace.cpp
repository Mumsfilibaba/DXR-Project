#include "Application/Text/FixedWidthFontFace.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Math/Math.h"

FFixedWidthFontFace::FFixedWidthFontFace(int32 InCharacterAdvance, int32 InLineHeight)
    : CharacterAdvance(Math::Max(1, InCharacterAdvance))
    , LineHeight(Math::Max(1, InLineHeight))
{
}

FFixedWidthFontFace::~FFixedWidthFontFace() = default;

const FFontAtlas* FFixedWidthFontFace::GetAtlas() const
{
    return nullptr;
}

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

int32 FFixedWidthFontFace::GetCapHeight() const
{
    return GetAscent();
}

const FShapedRun& FFixedWidthFontFace::ShapeText(const FStringView& Text) const
{
    ShapedRun.Glyphs.Clear();
    ShapedRun.Glyphs.Reserve(Text.Length());

    int32 Index = 0;
    while (Index < Text.Length())
    {
        int32 NumUnits = 0;

        FShapedGlyph& Shaped = ShapedRun.Glyphs.Emplace();
        Shaped.Codepoint   = static_cast<int32>(FStringConversion::DecodeCodePoint(Text.Data() + Index, Text.Length() - Index, NumUnits));
        Shaped.Offset      = (ShapedRun.Glyphs.Size() - 1) * CharacterAdvance;
        Shaped.Advance     = CharacterAdvance;
        Shaped.SourceIndex = Index;

        Index += NumUnits;
    }

    ShapedRun.Width = ShapedRun.Glyphs.Size() * CharacterAdvance;
    return ShapedRun;
}
