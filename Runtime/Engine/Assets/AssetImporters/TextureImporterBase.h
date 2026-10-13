#pragma once
#include "Engine/Assets/ITextureImporter.h"

struct FTextureImporterBase : public ITextureImporter
{
    virtual ~FTextureImporterBase() = default;

    virtual TSharedPtr<FTexture> ImportFromFile(const FStringView& FileName) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
