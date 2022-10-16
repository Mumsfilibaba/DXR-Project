#pragma once
#include "ITextureImporter.h"

class FTextureImporterBase
    : public ITextureImporter
{
public:
    FTextureImporterBase()  = default;
    ~FTextureImporterBase() = default;

    virtual FTexture* ImportFromFile(const FStringView& FileName) override final;

    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};