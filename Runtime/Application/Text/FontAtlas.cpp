#include "Application/Text/FontAtlas.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(Size, UserData) ((void)(UserData), Memory::Malloc(Size))
#define STBTT_free(Pointer, UserData) ((void)(UserData), Memory::Free(Pointer))
#include <stb_truetype.h>

// The atlas is square and doubles until every glyph fits, so a large pixel height still packs
static constexpr int32 GMinAtlasSize = 128;
static constexpr int32 GMaxAtlasSize = 4096;

FFontAtlas::FFontAtlas()
    : Pixels()
    , Glyphs()
    , Width(0)
    , Height(0)
    , LineHeight(0)
    , Ascent(0)
    , Descent(0)
    , CapHeight(0)
    , Revision(0)
{
}

FFontAtlas::~FFontAtlas() = default;

bool FFontAtlas::Build(const TArray<uint8>& FontData, int32 PixelHeight)
{
    Pixels.Clear(true);

    Width      = 0;
    Height     = 0;
    LineHeight = 0;
    Ascent     = 0;
    Descent    = 0;
    CapHeight  = 0;

    for (FGlyph& Glyph : Glyphs)
    {
        Glyph = FGlyph();
    }

    if (FontData.IsEmpty() || PixelHeight <= 0)
    {
        return false;
    }

    stbtt_fontinfo FontInfo;
    const int32 FontOffset = stbtt_GetFontOffsetForIndex(FontData.Data(), 0);
    if (FontOffset < 0 || !stbtt_InitFont(&FontInfo, FontData.Data(), FontOffset))
    {
        LOG_ERROR("[FFontAtlas]: Failed to parse the font data");
        return false;
    }

    const float ScaleFactor = stbtt_ScaleForPixelHeight(&FontInfo, static_cast<float>(PixelHeight));

    int32 UnscaledAscent  = 0;
    int32 UnscaledDescent = 0;
    int32 UnscaledLineGap = 0;
    stbtt_GetFontVMetrics(&FontInfo, &UnscaledAscent, &UnscaledDescent, &UnscaledLineGap);

    Ascent  = Math::CeilToInt(static_cast<float>(UnscaledAscent) * ScaleFactor);
    Descent = Math::CeilToInt(static_cast<float>(-UnscaledDescent) * ScaleFactor);

    // The gap belongs between the lines rather than inside them, which is what stb documents
    LineHeight = Math::CeilToInt(static_cast<float>(UnscaledAscent - UnscaledDescent + UnscaledLineGap) * ScaleFactor);

    for (int32 AtlasSize = GMinAtlasSize; AtlasSize <= GMaxAtlasSize; AtlasSize *= 2)
    {
        if (PackAtSize(FontData, PixelHeight, AtlasSize, AtlasSize))
        {
            Width  = AtlasSize;
            Height = AtlasSize;

            // Measured off a rasterized capital rather than read from OS/2, which stb does not parse.
            // A face whose 'H' did not rasterize falls back to the ascent, which only costs the optical
            // centring its correction rather than placing the text anywhere wrong.
            const int32 MeasuredCapHeight = -GetGlyph('H').BearingY;
            CapHeight = (MeasuredCapHeight > 0 && MeasuredCapHeight <= Ascent) ? MeasuredCapHeight : Ascent;

            ++Revision;
            return true;
        }
    }

    LOG_ERROR("[FFontAtlas]: Failed to pack the font at %d pixels into an atlas of at most %d texels", PixelHeight, GMaxAtlasSize);
    return false;
}

bool FFontAtlas::PackAtSize(const TArray<uint8>& FontData, int32 PixelHeight, int32 InWidth, int32 InHeight)
{
    const int32 TexelCount = InWidth * InHeight;

    TArray<uint8> Coverage;
    Coverage.Resize(TexelCount);

    Memory::Memzero(Coverage.Data(), static_cast<uint64>(TexelCount));

    stbtt_pack_context PackContext;
    if (!stbtt_PackBegin(&PackContext, Coverage.Data(), InWidth, InHeight, 0, 1, nullptr))
    {
        return false;
    }

    TStaticArray<stbtt_packedchar, GlyphCount> PackedGlyphs;
    const int32 PackResult = stbtt_PackFontRange(
        &PackContext,
        FontData.Data(),
        0,
        static_cast<float>(PixelHeight),
        FirstCodepoint,
        GlyphCount,
        PackedGlyphs.Data());

    stbtt_PackEnd(&PackContext);

    if (!PackResult)
    {
        return false;
    }

    Pixels.Resize(TexelCount * 4);

    uint8* RESTRICT DstTexel = Pixels.Data();
    for (int32 Index = 0; Index < TexelCount; ++Index)
    {
        DstTexel[0] = 255;
        DstTexel[1] = 255;
        DstTexel[2] = 255;
        DstTexel[3] = Coverage[Index];
        DstTexel += 4;
    }

    for (int32 Index = 0; Index < GlyphCount; ++Index)
    {
        const stbtt_packedchar& PackedGlyph = PackedGlyphs[Index];

        FGlyph& Glyph = Glyphs[Index];
        Glyph.AtlasRectangle = FRectangle(
            IntVector2(static_cast<int32>(PackedGlyph.x0), static_cast<int32>(PackedGlyph.y0)),
            static_cast<int32>(PackedGlyph.x1) - static_cast<int32>(PackedGlyph.x0),
            static_cast<int32>(PackedGlyph.y1) - static_cast<int32>(PackedGlyph.y0));

        Glyph.BearingX = Math::RoundToInt(PackedGlyph.xoff);
        Glyph.BearingY = Math::RoundToInt(PackedGlyph.yoff);
        Glyph.Advance  = Math::RoundToInt(PackedGlyph.xadvance);
    }

    return true;
}

const FGlyph& FFontAtlas::GetGlyph(CHAR Character) const
{
    const int32 Codepoint = static_cast<int32>(static_cast<uint8>(Character));
    if (Codepoint < FirstCodepoint || Codepoint >= LastCodepoint)
    {
        return Glyphs[0];
    }

    return Glyphs[Codepoint - FirstCodepoint];
}
