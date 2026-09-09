#include "Application/Text/TrueTypeFontFace.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"

static FORCEINLINE int32 ToCodepoint(CHAR Character)
{
    return static_cast<int32>(static_cast<uint8>(Character));
}

static uint64 HashText(const StringView& Text)
{
    uint64 Hash = 14695981039346656037ull;
    for (int32 Index = 0; Index < Text.Length(); ++Index)
    {
        Hash ^= static_cast<uint64>(static_cast<uint8>(Text[Index]));
        Hash *= 1099511628211ull;
    }

    return Hash;
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
    FCachedRun& Cached = ShapedRuns[static_cast<int32>(HashText(Text) % ShapedRunCacheSize)];

    const bool bIsHit = Cached.Revision == Atlas.GetRevision()
        && Cached.Text.Length() == Text.Length()
        && Memory::Memcmp(Cached.Text.Data(), Text.Data(), static_cast<uint64>(Text.Length())) == 0;

    if (bIsHit)
    {
        return Cached.Run;
    }

    ShapeRun(Text, Cached.Run);

    Cached.Text     = String(Text.Data(), Text.Length());
    Cached.Revision = Atlas.GetRevision();
    return Cached.Run;
}

void FTrueTypeFontFace::ShapeRun(const StringView& Text, FShapedRun& OutRun) const
{
    OutRun.Glyphs.Clear();
    OutRun.Width = 0;

    for (int32 Index = 0; Index < Text.Length(); ++Index)
    {
        Atlas.GetGlyph(ToCodepoint(Text[Index]));
    }

    OutRun.Glyphs.Reserve(Text.Length());

    int32 Pen = 0;
    for (int32 Index = 0; Index < Text.Length(); ++Index)
    {
        const int32 Codepoint = ToCodepoint(Text[Index]);
        const FGlyph& Glyph   = Atlas.GetGlyph(Codepoint);

        FShapedGlyph& Shaped = OutRun.Glyphs.Emplace();
        Shaped.Offset      = Pen;
        Shaped.SourceIndex = Index;
        Shaped.Glyph       = &Glyph;
        Shaped.Advance     = Glyph.Advance;

        if (Index + 1 < Text.Length())
        {
            Shaped.Advance += Atlas.GetKerning(Codepoint, ToCodepoint(Text[Index + 1]));
        }

        Pen += Shaped.Advance;
    }

    OutRun.Width = Pen;
}
