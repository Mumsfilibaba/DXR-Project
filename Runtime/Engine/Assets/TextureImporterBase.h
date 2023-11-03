#pragma once
#include "ITextureImporter.h"

struct FTextureImporterBase : public ITextureImporter
{
    virtual FTexture* ImportFromFile(const FStringView& FileName) override final;

    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};