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

static const FGlyph GEmptyGlyph;

struct FFontAtlas::FPage
{
    FPage()
        : Glyphs()
    {
        Glyphs.Resize(CodepointsPerPage);
    }

    TArray<FGlyph> Glyphs;
};

struct FFontAtlas::FPackState
{
    FPackState()
        : FontInfo()
        , Context()
        , ScaleFactor(0.0f)
        , bIsPacking(false)
    {
    }

    ~FPackState()
    {
        Close();
    }

    void Close()
    {
        if (bIsPacking)
        {
            stbtt_PackEnd(&Context);
            bIsPacking = false;
        }
    }

    stbtt_fontinfo     FontInfo;
    stbtt_pack_context Context;
    float              ScaleFactor;
    bool               bIsPacking;
};

FFontAtlas::FFontAtlas()
    : Pixels()
    , FontData()
    , PixelHeight(0)
    , Width(0)
    , Height(0)
    , LineHeight(0)
    , Ascent(0)
    , Descent(0)
    , CapHeight(0)
    , Coverage()
    , Pages()
    , UnpackablePages()
    , PackState()
    , Revision(0)
{
}

FFontAtlas::~FFontAtlas() = default;

void FFontAtlas::Reset()
{
    PackState.Reset();
    Pages.Clear();
    UnpackablePages.Clear();
    Pixels.Clear(true);
    Coverage.Clear(true);

    Width      = 0;
    Height     = 0;
    LineHeight = 0;
    Ascent     = 0;
    Descent    = 0;
    CapHeight  = 0;
}

bool FFontAtlas::Build(const TArray<uint8>& InFontData, int32 InPixelHeight)
{
    Reset();

    FontData    = InFontData;
    PixelHeight = InPixelHeight;

    if (FontData.IsEmpty() || PixelHeight <= 0)
    {
        return false;
    }

    TUniquePtr<FPackState> NewPackState = MakeUniquePtr<FPackState>();

    const int32 FontOffset = stbtt_GetFontOffsetForIndex(FontData.Data(), 0);
    if (FontOffset < 0 || !stbtt_InitFont(&NewPackState->FontInfo, FontData.Data(), FontOffset))
    {
        LOG_ERROR("[FFontAtlas]: Failed to parse the font data");
        return false;
    }

    NewPackState->ScaleFactor = stbtt_ScaleForPixelHeight(&NewPackState->FontInfo, static_cast<float>(PixelHeight));

    PackState = ::Move(NewPackState);

    int32 UnscaledAscent  = 0;
    int32 UnscaledDescent = 0;
    int32 UnscaledLineGap = 0;
    stbtt_GetFontVMetrics(&PackState->FontInfo, &UnscaledAscent, &UnscaledDescent, &UnscaledLineGap);

    Ascent     = Math::CeilToInt(static_cast<float>(UnscaledAscent) * PackState->ScaleFactor);
    Descent    = Math::CeilToInt(static_cast<float>(-UnscaledDescent) * PackState->ScaleFactor);
    LineHeight = Math::CeilToInt(static_cast<float>(UnscaledAscent - UnscaledDescent + UnscaledLineGap) * PackState->ScaleFactor);

    for (int32 AtlasSize = GMinAtlasSize; AtlasSize <= GMaxAtlasSize; AtlasSize *= 2)
    {
        if (PackAtSize(AtlasSize, AtlasSize))
        {
            const int32 MeasuredCapHeight = -GetGlyph('H').BearingY;
            CapHeight = (MeasuredCapHeight > 0 && MeasuredCapHeight <= Ascent) ? MeasuredCapHeight : Ascent;

            ++Revision;
            return true;
        }
    }

    LOG_ERROR("[FFontAtlas]: Failed to pack the font at %d pixels into an atlas of at most %d texels", PixelHeight, GMaxAtlasSize);

    Reset();
    return false;
}

bool FFontAtlas::PackPage(int32 PageIndex) const
{
    const int32 FirstOfPage = PageIndex * CodepointsPerPage;
    const int32 RangeStart  = Math::Max(FirstOfPage, FirstCodepoint);
    const int32 RangeCount  = (FirstOfPage + CodepointsPerPage) - RangeStart;

    TArray<stbtt_packedchar> PackedGlyphs;
    PackedGlyphs.Resize(RangeCount);

    const int32 PackResult = stbtt_PackFontRange(
        &PackState->Context,
        FontData.Data(),
        0,
        static_cast<float>(PixelHeight),
        RangeStart,
        RangeCount,
        PackedGlyphs.Data());

    if (!PackResult)
    {
        return false;
    }

    TUniquePtr<FPage> NewPage = MakeUniquePtr<FPage>();

    for (int32 Index = 0; Index < RangeCount; ++Index)
    {
        const stbtt_packedchar& PackedGlyph = PackedGlyphs[Index];

        FGlyph& Glyph = NewPage->Glyphs[(RangeStart - FirstOfPage) + Index];
        Glyph.AtlasRectangle = FRectangle(
            IntVector2(static_cast<int32>(PackedGlyph.x0), static_cast<int32>(PackedGlyph.y0)),
            static_cast<int32>(PackedGlyph.x1) - static_cast<int32>(PackedGlyph.x0),
            static_cast<int32>(PackedGlyph.y1) - static_cast<int32>(PackedGlyph.y0));

        Glyph.BearingX = Math::RoundToInt(PackedGlyph.xoff);
        Glyph.BearingY = Math::RoundToInt(PackedGlyph.yoff);
        Glyph.Advance  = Math::RoundToInt(PackedGlyph.xadvance);
    }

    Pages.Add(PageIndex, ::Move(NewPage));
    return true;
}

bool FFontAtlas::PackAtSize(int32 InWidth, int32 InHeight)
{
    TArray<int32> PageIndices;
    for (const auto& Page : Pages)
    {
        PageIndices.Add(Page.First);
    }

    if (!PageIndices.Contains(0))
    {
        PageIndices.Add(0);
    }

    PageIndices.Sort();

    PackState->Close();
    Pages.Clear();

    const int32 TexelCount = InWidth * InHeight;

    Coverage.Resize(TexelCount);
    Memory::Memzero(Coverage.Data(), static_cast<uint64>(TexelCount));

    if (!stbtt_PackBegin(&PackState->Context, Coverage.Data(), InWidth, InHeight, 0, 1, nullptr))
    {
        return false;
    }

    PackState->bIsPacking = true;

    Width  = InWidth;
    Height = InHeight;

    for (int32 PageIndex : PageIndices)
    {
        if (!PackPage(PageIndex))
        {
            PackState->Close();
            Pages.Clear();
            return false;
        }
    }

    ExpandCoverage();
    return true;
}

bool FFontAtlas::RasterizePage(int32 PageIndex) const
{
    if (UnpackablePages.Contains(PageIndex))
    {
        return false;
    }

    FFontAtlas* MutableThis = const_cast<FFontAtlas*>(this);
    if (PackPage(PageIndex))
    {
        MutableThis->ExpandCoverage();

        ++Revision;
        return true;
    }

    TArray<int32> PackedPages;
    for (const auto& Page : Pages)
    {
        PackedPages.Add(Page.First);
    }

    const int32 PackedSize = Width;
    Pages.Add(PageIndex, MakeUniquePtr<FPage>());

    for (int32 AtlasSize = PackedSize * 2; AtlasSize <= GMaxAtlasSize; AtlasSize *= 2)
    {
        if (MutableThis->PackAtSize(AtlasSize, AtlasSize))
        {
            ++Revision;
            return true;
        }
    }

    LOG_ERROR("[FFontAtlas]: Failed to grow past %d texels for codepoint page %d, which will draw as spaces", GMaxAtlasSize, PageIndex);

    UnpackablePages.Add(PageIndex);

    for (int32 PackedPageIndex : PackedPages)
    {
        Pages.Add(PackedPageIndex, MakeUniquePtr<FPage>());
    }

    MutableThis->PackAtSize(PackedSize, PackedSize);

    ++Revision;
    return false;
}

void FFontAtlas::ExpandCoverage()
{
    const int32 TexelCount = Width * Height;
    if (TexelCount <= 0)
    {
        return;
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
}

const FGlyph* FFontAtlas::FindGlyph(int32 Codepoint) const
{
    if (Codepoint < FirstCodepoint || Codepoint >= MaxCodepoint || !PackState || !PackState->bIsPacking)
    {
        return nullptr;
    }

    const int32 PageIndex = Codepoint / CodepointsPerPage;
    if (!Pages.Contains(PageIndex) && !RasterizePage(PageIndex))
    {
        return nullptr;
    }

    const TUniquePtr<FPage>* Page = Pages.Find(PageIndex);
    if (!Page)
    {
        return nullptr;
    }

    const FGlyph& Glyph = (*Page)->Glyphs[Codepoint % CodepointsPerPage];
    return (Glyph.Advance > 0 || !Glyph.AtlasRectangle.IsEmpty()) ? &Glyph : nullptr;
}

const FGlyph& FFontAtlas::GetGlyph(int32 Codepoint) const
{
    if (const FGlyph* Glyph = FindGlyph(Codepoint))
    {
        return *Glyph;
    }

    if (Codepoint != ' ')
    {
        if (const FGlyph* Space = FindGlyph(' '))
        {
            return *Space;
        }
    }

    return GEmptyGlyph;
}

int32 FFontAtlas::GetKerning(int32 Codepoint, int32 NextCodepoint) const
{
    if (!PackState || !PackState->bIsPacking)
    {
        return 0;
    }

    stbtt_fontinfo& FontInfo = PackState->FontInfo;

    int32 UnscaledKerning = stbtt_GetCodepointKernAdvance(&FontInfo, Codepoint, NextCodepoint);
    if (UnscaledKerning == 0 && FontInfo.kern && FontInfo.gpos)
    {
        const int32 GposOffset = FontInfo.gpos;

        FontInfo.gpos    = 0;
        UnscaledKerning  = stbtt_GetCodepointKernAdvance(&FontInfo, Codepoint, NextCodepoint);
        FontInfo.gpos    = GposOffset;
    }

    if (UnscaledKerning == 0)
    {
        return 0;
    }

    return Math::RoundToInt(static_cast<float>(UnscaledKerning) * PackState->ScaleFactor);
}
