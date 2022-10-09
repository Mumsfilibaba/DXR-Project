#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

struct FTexture;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ITextureImporter

struct ITextureImporter
{
    virtual ~ITextureImporter() = default;

    /**
     * @breif: Imports a texture with the specified filename
     * @return: Returns a pointer to the imported texture. Returns nullptr on failure.
     */
    virtual FTexture* ImportFromFile(const FStringView& FileName) = 0;

    /** @Return: Returns true if the FileName matches extension for this importer */
    virtual bool MatchExtenstion(const FStringView& FileName) = 0;
};