#pragma once
#include "ITextureImporter.h"

class FTextureImporterDDS
    : public ITextureImporter
{
public:
    FTextureImporterDDS()  = default;
    ~FTextureImporterDDS() = default;

    virtual FTexture* ImportFromFile(const FStringView& FileName) override final;

    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};