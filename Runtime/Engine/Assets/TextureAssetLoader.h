#pragma once
#include "AssetCore/AssetRegistry.h"
#include "AssetCore/TextureAsset.h"
#include "Engine/Resources/Texture.h"

struct ENGINE_API TextureAssetLoader
{
    /** @return Returns the RHI format of Format, its sRGB form when bSRGB and it has one, or EFormat::Unknown */
    NODISCARD static EFormat GetRHIFormat(ETextureFormat Format, bool bSRGB);

    /** @brief Load a texture asset into an FTexture2D with its RHI texture, as the loader FAssetLoader calls for textures */
    static TSharedPtr<FTexture> Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError);
};
