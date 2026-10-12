#include "Application/Text/FontAtlas.h"
#include "Core/Algorithms/Algorithm.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Threading/ScopedLock.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(Size, UserData) ((void)(UserData), Memory::Malloc(Size))
#define STBTT_free(Pointer, UserData) ((void)(UserData), Memory::Free(Pointer))
#include <stb_truetype.h>

// The atlas is square and doubles until every glyph fits, so a large pixel height still packs
constexpr int32 MIN_ATLAS_SIZE = 128;
constexpr int32 MAX_ATLAS_SIZE = 4096;

// Code points below this are controls when the font lacks them, and not worth loading a fallback font for
static constexpr int32 FIRST_FALLBACK_CODEPOINT = 0xA0;

static const FGlyph GEmptyGlyph;

struct FFallbackFont
{
    FFallbackFont(const FString& InFilename)
        : Filename(InFilename)
        , FontData()
        , FontInfo()
        , bIsLoaded(false)
        , bIsValid(false)
    {
    }

    FString        Filename;
    TArray<uint8>  FontData;
    stbtt_fontinfo FontInfo;
    bool           bIsLoaded;
    bool           bIsValid;
};

class FFallbackFonts
{
public:
    static FFallbackFonts& Get()
    {
        static FFallbackFonts Instance;
        return Instance;
    }

public:
    const FFallbackFont* FindFontFor(int32 Codepoint)
    {
        SCOPED_LOCK(Section);

        for (TUniquePtr<FFallbackFont>& Font : Fonts)
        {
            if (!Font->bIsLoaded)
            {
                Load(*Font);
            }

            if (Font->bIsValid && stbtt_FindGlyphIndex(&Font->FontInfo, Codepoint) != 0)
            {
                return Font.Get();
            }
        }

        return nullptr;
    }

private:
    static void Load(FFallbackFont& Font)
    {
        Font.bIsLoaded = true;

        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Font.Filename);
        if (!FileHandle || !File::ReadFile(FileHandle.Get(), Font.FontData) || Font.FontData.IsEmpty())
        {
            Font.FontData.Clear(true);
            return;
        }

        const int32 FontOffset = stbtt_GetFontOffsetForIndex(Font.FontData.Data(), 0);
        if (FontOffset < 0 || !stbtt_InitFont(&Font.FontInfo, Font.FontData.Data(), FontOffset))
        {
            LOG_WARNING("[FFontAtlas]: Failed to parse the fallback font '%s'", *Font.Filename);
            Font.FontData.Clear(true);
            return;
        }

        Font.bIsValid = true;
        LOG_INFO("[FFontAtlas]: Loaded the fallback font '%s'", *Font.Filename);
    }

    FFallbackFonts()
    {
    #if PLATFORM_WINDOWS
        FString WindowsDirectory;
        if (!FPlatformMisc::GetEnvironmentVariable(TEXT("WINDIR"), WindowsDirectory) || WindowsDirectory.IsEmpty())
        {
            WindowsDirectory = TEXT("C:\\Windows");
        }

        // Japanese first, since Yu Gothic also covers the ideographs Chinese shares, then Chinese and Korean
        const FString FontDirectory = WindowsDirectory + TEXT("\\Fonts\\");
        Fonts.Add(MakeUniquePtr<FFallbackFont>(FontDirectory + TEXT("YuGothM.ttc")));
        Fonts.Add(MakeUniquePtr<FFallbackFont>(FontDirectory + TEXT("msyh.ttc")));
        Fonts.Add(MakeUniquePtr<FFallbackFont>(FontDirectory + TEXT("malgun.ttf")));
    #elif PLATFORM_MACOS
        // Hiragino Kaku Gothic W3, whose file name is Japanese
        Fonts.Add(MakeUniquePtr<FFallbackFont>(FString(TEXT("/System/Library/Fonts/\u30D2\u30E9\u30AE\u30CE\u89D2\u30B4\u30B7\u30C3\u30AF W3.ttc"))));
        Fonts.Add(MakeUniquePtr<FFallbackFont>(FString(TEXT("/System/Library/Fonts/PingFang.ttc"))));
        Fonts.Add(MakeUniquePtr<FFallbackFont>(FString(TEXT("/System/Library/Fonts/Hiragino Sans GB.ttc"))));
        Fonts.Add(MakeUniquePtr<FFallbackFont>(FString(TEXT("/System/Library/Fonts/AppleSDGothicNeo.ttc"))));
    #endif
    }

    FCriticalSection                  Section;
    TArray<TUniquePtr<FFallbackFont>> Fonts;
};

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
        , EmSize(0.0f)
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
    float              EmSize;
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
    , FirstPage(nullptr)
    , Revision(0)
    , LayoutRevision(0)
    , ChangedSinceLayout()
{
}

FFontAtlas::~FFontAtlas() = default;

void FFontAtlas::ClearPages() const
{
    Pages.Clear();
    FirstPage = nullptr;
}

void FFontAtlas::MarkLayoutChanged() const
{
    ++Revision;
    LayoutRevision     = Revision;
    ChangedSinceLayout = FRectangle();
}

void FFontAtlas::Reset()
{
    PackState.Reset();
    ClearPages();
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
    NewPackState->EmSize      = NewPackState->ScaleFactor / stbtt_ScaleForMappingEmToPixels(&NewPackState->FontInfo, 1.0f);

    PackState = ::Move(NewPackState);

    int32 UnscaledAscent  = 0;
    int32 UnscaledDescent = 0;
    int32 UnscaledLineGap = 0;
    stbtt_GetFontVMetrics(&PackState->FontInfo, &UnscaledAscent, &UnscaledDescent, &UnscaledLineGap);

    Ascent     = Math::CeilToInt(static_cast<float>(UnscaledAscent) * PackState->ScaleFactor);
    Descent    = Math::CeilToInt(static_cast<float>(-UnscaledDescent) * PackState->ScaleFactor);
    LineHeight = Math::CeilToInt(static_cast<float>(UnscaledAscent - UnscaledDescent + UnscaledLineGap) * PackState->ScaleFactor);

    for (int32 AtlasSize = MIN_ATLAS_SIZE; AtlasSize <= MAX_ATLAS_SIZE; AtlasSize *= 2)
    {
        if (PackAtSize(AtlasSize, AtlasSize))
        {
            const int32 MeasuredCapHeight = -GetGlyph('H').BearingY;
            CapHeight = (MeasuredCapHeight > 0 && MeasuredCapHeight <= Ascent) ? MeasuredCapHeight : Ascent;

            MarkLayoutChanged();
            return true;
        }
    }

    LOG_ERROR("[FFontAtlas]: Failed to pack the font at %d pixels into an atlas of at most %d texels", PixelHeight, MAX_ATLAS_SIZE);

    Reset();
    return false;
}

bool FFontAtlas::PackPage(int32 PageIndex, FRectangle& OutPackedRegion) const
{
    struct FGlyphSource
    {
        const uint8*  FontData;
        float         FontSize;
        TArray<int32> Codepoints;
    };

    const int32 FirstOfPage = PageIndex * CodepointsPerPage;
    const int32 RangeStart  = Math::Max(FirstOfPage, FirstCodepoint);
    const int32 RangeEnd    = FirstOfPage + CodepointsPerPage;

    TArray<FGlyphSource> Sources;
    Sources.Add(FGlyphSource{ FontData.Data(), static_cast<float>(PixelHeight), TArray<int32>() });

    for (int32 Codepoint = RangeStart; Codepoint < RangeEnd; ++Codepoint)
    {
        const FFallbackFont* FallbackFont = nullptr;
        if (Codepoint >= FIRST_FALLBACK_CODEPOINT && stbtt_FindGlyphIndex(&PackState->FontInfo, Codepoint) == 0)
        {
            FallbackFont = FFallbackFonts::Get().FindFontFor(Codepoint);
        }

        FGlyphSource* Source = &Sources[0];
        if (FallbackFont)
        {
            Source = nullptr;
            for (FGlyphSource& Candidate : Sources)
            {
                if (Candidate.FontData == FallbackFont->FontData.Data())
                {
                    Source = &Candidate;
                    break;
                }
            }

            if (!Source)
            {
                Source = &Sources.Add(FGlyphSource{ FallbackFont->FontData.Data(), STBTT_POINT_SIZE(PackState->EmSize), TArray<int32>() });
            }
        }

        Source->Codepoints.Add(Codepoint);
    }

    TUniquePtr<FPage> NewPage = MakeUniquePtr<FPage>();

    for (FGlyphSource& Source : Sources)
    {
        if (Source.Codepoints.IsEmpty())
        {
            continue;
        }

        TArray<stbtt_packedchar> PackedGlyphs;
        PackedGlyphs.Resize(Source.Codepoints.Size());

        stbtt_pack_range Range = {};
        Range.font_size                   = Source.FontSize;
        Range.array_of_unicode_codepoints = Source.Codepoints.Data();
        Range.num_chars                   = Source.Codepoints.Size();
        Range.chardata_for_range          = PackedGlyphs.Data();

        if (!stbtt_PackFontRanges(&PackState->Context, Source.FontData, 0, &Range, 1))
        {
            return false;
        }

        for (int32 Index = 0; Index < Source.Codepoints.Size(); ++Index)
        {
            const stbtt_packedchar& PackedGlyph = PackedGlyphs[Index];

            FGlyph& Glyph = NewPage->Glyphs[Source.Codepoints[Index] - FirstOfPage];
            Glyph.AtlasRectangle = FRectangle(
                FIntVector2(static_cast<int32>(PackedGlyph.x0), static_cast<int32>(PackedGlyph.y0)),
                static_cast<int32>(PackedGlyph.x1) - static_cast<int32>(PackedGlyph.x0),
                static_cast<int32>(PackedGlyph.y1) - static_cast<int32>(PackedGlyph.y0));

            Glyph.BearingX = Math::RoundToInt(PackedGlyph.xoff);
            Glyph.BearingY = Math::RoundToInt(PackedGlyph.yoff);
            Glyph.Advance  = Math::RoundToInt(PackedGlyph.xadvance);

            OutPackedRegion = OutPackedRegion.Union(Glyph.AtlasRectangle);
        }
    }

    if (PageIndex == 0)
    {
        FirstPage = NewPage.Get();
    }

    Pages.Add(PageIndex, ::Move(NewPage));
    return true;
}

bool FFontAtlas::PackAtSize(int32 InWidth, int32 InHeight)
{
    TArray<int32> PageIndices;
    for (const TPair<const int32&, TUniquePtr<FPage>&>& Page : Pages)
    {
        PageIndices.Add(Page.First);
    }

    if (!PageIndices.Contains(0))
    {
        PageIndices.Add(0);
    }

    Algorithm::Sort(PageIndices);

    PackState->Close();
    ClearPages();

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

    FRectangle PackedRegion;
    for (int32 PageIndex : PageIndices)
    {
        if (!PackPage(PageIndex, PackedRegion))
        {
            PackState->Close();
            ClearPages();
            return false;
        }
    }

    Pixels.Resize(TexelCount * 4);
    ExpandCoverage(FRectangle(FIntVector2(0, 0), Width, Height));
    return true;
}

bool FFontAtlas::RasterizePage(int32 PageIndex) const
{
    if (UnpackablePages.Contains(PageIndex))
    {
        return false;
    }

    FFontAtlas* MutableThis  = const_cast<FFontAtlas*>(this);
    FRectangle  PackedRegion;
    if (PackPage(PageIndex, PackedRegion))
    {
        ExpandCoverage(PackedRegion);
        ChangedSinceLayout = ChangedSinceLayout.Union(PackedRegion);

        ++Revision;
        return true;
    }

    TArray<int32> PackedPages;
    for (const TPair<const int32&, TUniquePtr<FPage>&>& Page : Pages)
    {
        PackedPages.Add(Page.First);
    }

    const int32 PackedSize = Width;
    Pages.Add(PageIndex, MakeUniquePtr<FPage>());

    for (int32 AtlasSize = PackedSize * 2; AtlasSize <= MAX_ATLAS_SIZE; AtlasSize *= 2)
    {
        if (MutableThis->PackAtSize(AtlasSize, AtlasSize))
        {
            MarkLayoutChanged();
            return true;
        }
    }

    LOG_ERROR("[FFontAtlas]: Failed to grow past %d texels for codepoint page %d, which will draw as spaces", MAX_ATLAS_SIZE, PageIndex);

    UnpackablePages.Add(PageIndex);

    for (int32 PackedPageIndex : PackedPages)
    {
        Pages.Add(PackedPageIndex, MakeUniquePtr<FPage>());
    }

    MutableThis->PackAtSize(PackedSize, PackedSize);

    MarkLayoutChanged();
    return false;
}

void FFontAtlas::ExpandCoverage(const FRectangle& Region) const
{
    const FRectangle Clamped = Region.Intersect(FRectangle(FIntVector2(0, 0), Width, Height));
    if (Clamped.IsEmpty() || Pixels.Size() < Width * Height * 4)
    {
        return;
    }

    for (int32 Row = Clamped.Position.Y; Row < Clamped.GetBottom(); ++Row)
    {
        const int32     RowStart = (Row * Width) + Clamped.Position.X;
        const uint8*    SrcTexel = Coverage.Data() + RowStart;
        uint8* RESTRICT DstTexel = Pixels.Data() + (RowStart * 4);

        for (int32 Column = 0; Column < Clamped.Width; ++Column)
        {
            DstTexel[0] = 255;
            DstTexel[1] = 255;
            DstTexel[2] = 255;
            DstTexel[3] = SrcTexel[Column];
            DstTexel += 4;
        }
    }
}

const FGlyph* FFontAtlas::FindGlyph(int32 Codepoint) const
{
    if (static_cast<uint32>(Codepoint) < static_cast<uint32>(CodepointsPerPage) && FirstPage)
    {
        const FGlyph& Glyph = FirstPage->Glyphs[Codepoint];
        return (Glyph.Advance > 0 || !Glyph.AtlasRectangle.IsEmpty()) ? &Glyph : nullptr;
    }

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
