#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Application/Text/FontAtlas.h"
#include "Application/Text/IFontFace.h"

class APPLICATION_API FTrueTypeFontFace final : public IFontFace
{
public:

    /**
     * @brief Loads a TrueType file and rasterizes it at one size.
     *
     * @param Filename    The path to the font file.
     * @param PixelHeight The height to rasterize at, in pixels.
     * @return The face, or null when the file could not be read or packed.
     */
    static TSharedPtr<FTrueTypeFontFace> CreateFromFile(const String& Filename, int32 PixelHeight);

public:
    FTrueTypeFontFace();
    virtual ~FTrueTypeFontFace();

    // IFontFace Interface
    virtual const FFontAtlas* GetAtlas() const override final;
    virtual int32 GetLineHeight() const override final;
    virtual int32 GetAscent() const override final;
    virtual int32 GetDescent() const override final;
    virtual int32 GetCapHeight() const override final;
    virtual int32 GetCharacterAdvance(CHAR Character) const override final;
    virtual int32 MeasureWidth(const StringView& Text) const override final;
    virtual int32 FindCharacterIndexAtOffset(const StringView& Text, int32 OffsetX) const override final;

private:
    FFontAtlas Atlas;
};
