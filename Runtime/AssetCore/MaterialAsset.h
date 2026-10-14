#pragma once
#include "AssetCore/AssetRef.h"
#include "AssetCore/MaterialDescription.h"
#include "Core/Json/JsonValue.h"

class FJsonArchive;

struct ASSETCORE_API FMaterialAssetData
{
    void Serialize(FJsonArchive& Archive);

    FGuid               Guid;
    FMaterialInfo       Info;
    FAssetRef Textures[EMaterialTextureSlot::Count];

    /** @brief Reserved until emissive and vertex colours render (R0): read and written, but not used yet */
    FFloatColor         EmissiveColor        = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    float               EmissiveIntensity    = 1.0f;
    FAssetRef EmissiveTexture;
    bool                bMultiplyVertexColor = false;
};

struct ASSETCORE_API MaterialAssets
{
    static constexpr int32 CurrentVersion = 1;

    /** @brief Write Material in the text format, with defaults left out and its textures listed as Dependencies */
    NODISCARD static FJsonValue ToJson(const FMaterialAssetData& Material);

    /** @brief Read a material, failing with every error and its field path, such as "Routes.Roughness.Slot" */
    static bool FromJson(const FJsonValue& Value, FMaterialAssetData& OutMaterial, FString& OutError);

    static bool SaveToFile(const FString& Filename, const FMaterialAssetData& Material, FString& OutError);
    static bool LoadFromFile(const FString& Filename, FMaterialAssetData& OutMaterial, FString& OutError);

    /** @return Returns the name a texture slot has in the file, such as "BaseColor" */
    NODISCARD static const CHAR_T* GetTextureSlotName(EMaterialTextureSlot::Type Slot);
};
