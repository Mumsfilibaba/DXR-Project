#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/UniquePtr.h"
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

    /** @brief The first codepoint packed when the atlas is built. */
    static constexpr int32 FirstCodepoint = 32;

    /** @brief One past the last codepoint packed when the atlas is built. */
    static constexpr int32 LastCodepoint = 127;

    /** @brief The number of glyphs packed when the atlas is built. */
    static constexpr int32 GlyphCount = LastCodepoint - FirstCodepoint;

    /** @brief How many codepoints a page covers, which is the unit anything past the built range arrives in. */
    static constexpr int32 CodepointsPerPage = 128;

    /** @brief One past the highest codepoint that can be asked for, which is the end of the Basic Multilingual Plane. */
    static constexpr int32 MaxCodepoint = 0x10000;

public:
    FFontAtlas();
    ~FFontAtlas();

    FFontAtlas(const FFontAtlas&) = delete;
    FFontAtlas& operator=(const FFontAtlas&) = delete;

    /**
     * @brief Rasterizes the printable ASCII range of a font into the atlas, and keeps the font data so that
     * anything outside that range can be rasterized later. The atlas is grown until every glyph fits, so a
     * large pixel height does not silently drop glyphs.
     *
     * @param InFontData    The contents of a TrueType file.
     * @param InPixelHeight The height to rasterize at, in pixels.
     * @return True when the font parsed and every glyph was packed.
     */
    bool Build(const TArray<uint8>& InFontData, int32 InPixelHeight);

    /**
     * @brief Looks up a glyph, rasterizing the page holding it when it is asked for the first time. A page
     * that arrives this way bumps the revision, which is how a renderer learns its texture went stale, and
     * where it does not fit it lays every page out again on a bigger sheet. That last case moves the
     * glyphs, so a reference from an earlier call is only good until the next call that packs a page.
     *
     * @param Codepoint The codepoint to look up.
     * @return The glyph, or the glyph for space when the codepoint has none in this face.
     */
    NODISCARD const FGlyph& GetGlyph(int32 Codepoint) const;

    /**
     * @brief Gets the rasterized pixels, in the RGBA8 layout FTextureFactory expects.
     *
     * @return The pixels, white with the glyph coverage in alpha, in tightly packed rows of GetWidth()
     * texels, and only meaningful once IsValid() reports true.
     */
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

    /**
     * @brief Gets how tall a capital is.
     *
     * @return The distance in pixels from the baseline up to the top of the rasterized 'H', falling back
     * to the ascent when that glyph did not rasterize.
     */
    NODISCARD FORCEINLINE int32 GetCapHeight() const
    {
        return CapHeight;
    }

    /**
     * @brief Gets the counter a renderer watches to tell that its texture went stale.
     *
     * @return The revision, bumped by every Build that succeeded and by every page rasterized since.
     */
    NODISCARD FORCEINLINE uint64 GetRevision() const
    {
        return Revision;
    }

    /**
     * @brief Gets the kerning between two codepoints, which is the correction a pair carries over the sum
     * of their advances.
     *
     * @param Codepoint     The codepoint on the left.
     * @param NextCodepoint The codepoint on the right.
     * @return The correction in pixels, negative where the pair tucks together and zero where the face
     * has no kern table or no entry for the pair.
     */
    NODISCARD int32 GetKerning(int32 Codepoint, int32 NextCodepoint) const;

    /** @return True when a Build succeeded and there are pixels to upload, so the atlas can be drawn from. */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return !Pixels.IsEmpty();
    }

private:
    struct FPackState;
    struct FPage;

    bool PackAtSize(int32 InWidth, int32 InHeight);
    bool PackPage(int32 PageIndex) const;
    bool RasterizePage(int32 PageIndex) const;
    void ExpandCoverage();
    void Reset();

    NODISCARD const FGlyph* FindGlyph(int32 Codepoint) const;

    TArray<uint8>                          Pixels;
    TArray<uint8>                          FontData;
    int32                                  PixelHeight;
    int32                                  Width;
    int32                                  Height;
    int32                                  LineHeight;
    int32                                  Ascent;
    int32                                  Descent;
    int32                                  CapHeight;
    mutable TArray<uint8>                  Coverage;
    mutable TMap<int32, TUniquePtr<FPage>> Pages;
    mutable TSet<int32>                    UnpackablePages;
    mutable TUniquePtr<FPackState>         PackState;
    mutable uint64                         Revision;
};
