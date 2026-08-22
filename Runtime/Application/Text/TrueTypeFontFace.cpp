#include "Application/Text/TrueTypeFontFace.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"

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

int32 FTrueTypeFontFace::GetCharacterAdvance(CHAR Character) const
{
    return Atlas.GetGlyph(Character).Advance;
}

int32 FTrueTypeFontFace::MeasureWidth(const StringView& Text) const
{
    // A proportional face has to sum the advances, since there is no single character width to scale by
    int32 Width = 0;
    for (int32 Index = 0; Index < Text.Length(); ++Index)
    {
        Width += Atlas.GetGlyph(Text[Index]).Advance;
    }

    return Width;
}

int32 FTrueTypeFontFace::FindCharacterIndexAtOffset(const StringView& Text, int32 OffsetX) const
{
    if (OffsetX <= 0)
    {
        return 0;
    }

    int32 Pen = 0;
    for (int32 Index = 0; Index < Text.Length(); ++Index)
    {
        const int32 Advance = Atlas.GetGlyph(Text[Index]).Advance;

        // Break at the half-glyph so a click on the right half of a character lands after it
        if (OffsetX < Pen + (Advance / 2))
        {
            return Index;
        }

        Pen += Advance;
    }

    return Text.Length();
}
