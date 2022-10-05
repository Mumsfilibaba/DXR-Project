#pragma once
#include "ITextureImporter.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureImporterBase

class FTextureImporterBase
    : public ITextureImporter
{
public:
    FTextureImporterBase()  = default;
    ~FTextureImporterBase() = default;

    virtual FTextureResource* ImportFromFile(const FStringView& FileName) override final;

    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};