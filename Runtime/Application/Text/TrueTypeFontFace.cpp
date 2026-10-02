#include "Application/Text/TrueTypeFontFace.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/TypeHash.h"

static FORCEINLINE int32 ToCodepoint(CHAR Character)
{
    return static_cast<int32>(static_cast<uint8>(Character));
}

TSharedPtr<FTrueTypeFontFace> FTrueTypeFontFace::CreateFromFile(const String& Filename, int32 PixelHeight)
{
    TArray<uint8> FontData;
    {
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
        if (!FileHandle)
        {
            LOG_ERROR("[FTrueTypeFontFace]: Failed to open '%s'", *Filename);
            return nullptr;
        }

        if (!File::ReadFile(FileHandle.Get(), FontData))
        {
            LOG_ERROR("[FTrueTypeFontFace]: Failed to read '%s'", *Filename);
            return nullptr;
        }
    }

    TSharedPtr<FTrueTypeFontFace> NewFace = MakeSharedPtr<FTrueTypeFontFace>();
    if (!NewFace->Atlas.Build(FontData, PixelHeight))
    {
        LOG_ERROR("[FTrueTypeFontFace]: Failed to rasterize '%s' at %d pixels", *Filename, PixelHeight);
        return nullptr;
    }

    return NewFace;
}

FTrueTypeFontFace::FTrueTypeFontFace()
    : Atlas()
    , ShapedRuns()
    , ShapedRunReplacementWays()
{
}

FTrueTypeFontFace::~FTrueTypeFontFace() = default;

const FFontAtlas* FTrueTypeFontFace::GetAtlas() const
{
    return Atlas.IsValid() ? &Atlas : nullptr;
}

int32 FTrueTypeFontFace::GetLineHeight() const
{
    return Atlas.GetLineHeight();
}

int32 FTrueTypeFontFace::GetAscent() const
{
    return Atlas.GetAscent();
}

int32 FTrueTypeFontFace::GetDescent() const
{
    return Atlas.GetDescent();
}

int32 FTrueTypeFontFace::GetCapHeight() const
{
    return Atlas.GetCapHeight();
}

const FShapedRun& FTrueTypeFontFace::ShapeText(const StringView& Text) const
{
    const int32  SetIndex  = static_cast<int32>(HashBytes(Text.Data(), static_cast<uint64>(Text.Length())) & (ShapedRunCacheSetCount - 1));
    const int32  SetOffset = SetIndex * ShapedRunCacheWays;
    const uint64 Revision  = Atlas.GetRevision();

    for (int32 Way = 0; Way < ShapedRunCacheWays; ++Way)
    {
        FCachedRun& Candidate = ShapedRuns[SetOffset + Way];
        if (Candidate.Revision == Revision
            && Candidate.Text.Length() == Text.Length()
            && Memory::Memcmp(Candidate.Text.Data(), Text.Data(), static_cast<uint64>(Text.Length())) == 0)
        {
            return Candidate.Run;
        }
    }

    uint8& ReplacementWay = ShapedRunReplacementWays[SetIndex];
    FCachedRun& Cached = ShapedRuns[SetOffset + ReplacementWay];
    ReplacementWay = static_cast<uint8>((ReplacementWay + 1) % ShapedRunCacheWays);

    ShapeRun(Text, Cached.Run);

    // Shaping can pack pages, and the run is valid at the revision it ended on rather than the one it started from
    Cached.Text     = String(Text.Data(), Text.Length());
    Cached.Revision = Atlas.GetRevision();
    return Cached.Run;
}

void FTrueTypeFontFace::ShapeRun(const StringView& Text, FShapedRun& OutRun) const
{
    uint64 LayoutRevision = 0;
    do
    {
        LayoutRevision = Atlas.GetLayoutRevision();

        OutRun.Glyphs.Clear();
        OutRun.Glyphs.Reserve(Text.Length());

        int32 Pen = 0;
        for (int32 Index = 0; Index < Text.Length(); ++Index)
        {
            const int32   Codepoint = ToCodepoint(Text[Index]);
            const FGlyph& Glyph     = Atlas.GetGlyph(Codepoint);

            FShapedGlyph& Shaped = OutRun.Glyphs.Emplace();
            Shaped.Offset      = Pen;
            Shaped.SourceIndex = Index;
            Shaped.Codepoint   = Codepoint;
            Shaped.Advance     = Glyph.Advance;

            if (Index + 1 < Text.Length())
            {
                Shaped.Advance += Atlas.GetKerning(Codepoint, ToCodepoint(Text[Index + 1]));
            }

            Pen += Shaped.Advance;
        }

        OutRun.Width = Pen;
    }
    while (Atlas.GetLayoutRevision() != LayoutRevision);
}
