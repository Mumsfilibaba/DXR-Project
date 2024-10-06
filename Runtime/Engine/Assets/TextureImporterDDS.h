#pragma once
#include "ITextureImporter.h"

struct FTextureImporterDDS : public ITextureImporter
{
    virtual TSharedRef<FTexture> ImportFromFile(const FStringView& FileName) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
