#pragma once
#include "Application/Text/IFontFace.h"

class APPLICATION_API FFixedWidthFontFace final : public IFontFace
{
public:
    FFixedWidthFontFace(int32 InCharacterAdvance, int32 InLineHeight);
    virtual ~FFixedWidthFontFace();

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
    int32 CharacterAdvance;
    int32 LineHeight;
};
