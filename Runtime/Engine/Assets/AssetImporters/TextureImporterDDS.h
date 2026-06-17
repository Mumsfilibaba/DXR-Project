#pragma once
#include "Engine/Assets/ITextureImporter.h"

struct FTextureImporterDDS : public ITextureImporter
{
    virtual ~FTextureImporterDDS() = default;

    virtual TSharedRef<FTexture> ImportFromFile(const StringView& FileName) override final;
    virtual bool MatchExtenstion(const StringView& FileName) override final;
};
