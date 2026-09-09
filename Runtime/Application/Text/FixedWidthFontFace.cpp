#include "Application/Text/FixedWidthFontFace.h"
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

const FShapedRun& FFixedWidthFontFace::ShapeText(const StringView& Text) const
{
    ShapedRun.Glyphs.Clear();
    ShapedRun.Glyphs.Reserve(Text.Length());

    for (int32 Index = 0; Index < Text.Length(); ++Index)
    {
        FShapedGlyph& Shaped = ShapedRun.Glyphs.Emplace();
        Shaped.Offset      = Index * CharacterAdvance;
        Shaped.Advance     = CharacterAdvance;
        Shaped.SourceIndex = Index;
    }

    ShapedRun.Width = Text.Length() * CharacterAdvance;
    return ShapedRun;
}
