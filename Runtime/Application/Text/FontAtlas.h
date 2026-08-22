#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/StaticArray.h"
#include "Application/Layout/LayoutTypes.h"

struct FGlyph
{
    FGlyph()
        : AtlasRectangle()
        , BearingX(0)
        , BearingY(0)
        , Advance(0)
    {
    }

    /** @brief The region of the atlas holding this glyph, in pixels. */
    FRectangle AtlasRectangle;

    /** @brief Offset from the pen to the left edge of the glyph, in pixels. */
    int32 BearingX;

    /** @brief Offset from the baseline down to the top edge of the glyph, in pixels. */
    int32 BearingY;

    /** @brief How far the pen moves after this glyph, in pixels. */
    int32 Advance;
};

class APPLICATION_API FFontAtlas
{
public:

    /** @brief The first codepoint packed into the atlas. */
    static constexpr int32 FirstCodepoint = 32;

    /** @brief One past the last codepoint packed into the atlas. */
    static constexpr int32 LastCodepoint = 127;

    /** @brief The number of glyphs the atlas holds. */
    static constexpr int32 GlyphCount = LastCodepoint - FirstCodepoint;

public:
    FFontAtlas();
    ~FFontAtlas();

    /**
     * @brief Rasterizes the printable ASCII range of a font into the atlas. The atlas is grown until
     * every glyph fits, so a large pixel height does not silently drop glyphs.
     *
     * @param FontData    The contents of a TrueType file.
     * @param PixelHeight The height to rasterize at, in pixels.
     * @return True when the font parsed and every glyph was packed.
     */
    bool Build(const TArray<uint8>& FontData, int32 PixelHeight);

    /**
     * @brief Looks up a glyph.
     *
     * @param Character The character to look up.
     * @return The glyph, or the glyph for space when the character falls outside the packed range.
     */
    NODISCARD const FGlyph& GetGlyph(CHAR Character) const;

    /** @brief The pixels, RGBA8 with white color and the coverage in alpha, as FTextureFactory expects. */
    NODISCARD FORCEINLINE const uint8* GetPixels() const
    {
        return Pixels.Data();
    }

    NODISCARD FORCEINLINE int32 GetWidth() const
    {
        return Width;
    }

    NODISCARD FORCEINLINE int32 GetHeight() const
    {
        return Height;
    }

    NODISCARD FORCEINLINE int32 GetLineHeight() const
    {
        return LineHeight;
    }

    NODISCARD FORCEINLINE int32 GetAscent() const
    {
        return Ascent;
    }

    NODISCARD FORCEINLINE int32 GetDescent() const
    {
        return Descent;
    }

    /** @brief Distance from the baseline up to the top of a capital, measured off the rasterized 'H'. */
    NODISCARD FORCEINLINE int32 GetCapHeight() const
    {
        return CapHeight;
    }

    /** @brief Bumped by every Build, so a renderer can tell that its texture went stale. */
    NODISCARD FORCEINLINE uint64 GetRevision() const
    {
        return Revision;
    }

    /** @brief True when Build succeeded and there are pixels to upload. */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return !Pixels.IsEmpty();
    }

private:
    bool PackAtSize(const TArray<uint8>& FontData, int32 PixelHeight, int32 InWidth, int32 InHeight);

    TArray<uint8>                    Pixels;
    TStaticArray<FGlyph, GlyphCount> Glyphs;
    int32                            Width;
    int32                            Height;
    int32                            LineHeight;
    int32                            Ascent;
    int32                            Descent;
    int32                            CapHeight;
    uint64                           Revision;
};
