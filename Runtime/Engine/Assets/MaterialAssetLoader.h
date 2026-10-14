#pragma once
#include "AssetCore/AssetRegistry.h"
#include "Engine/Resources/Material.h"

struct ENGINE_API MaterialAssetLoader
{
    /**
     * @brief Load a .dxrmaterial into an FMaterial, loading its textures through the asset loader, as the loader FAssetLoader
     * calls for materials. A texture that can't be loaded is logged and left out, and the base colour falls back to the engine's.
     */
    static TSharedPtr<FMaterial> Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError);
};
