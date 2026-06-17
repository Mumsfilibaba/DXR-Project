#pragma once
#include "Engine/Assets/ITextureImporter.h"

struct FTextureImporterBase : public ITextureImporter
{
    virtual ~FTextureImporterBase() = default;

    virtual TSharedRef<FTexture> ImportFromFile(const StringView& FileName) override final;
    virtual bool MatchExtenstion(const StringView& FileName) override final;
};
