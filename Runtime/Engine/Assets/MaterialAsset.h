#pragma once
#include "AssetCore/AssetRef.h"
#include "AssetCore/AssetRegistry.h"
#include "Core/Json/JsonValue.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Texture.h"

struct ENGINE_API FMaterialAssetData
{
    void Serialize(FJsonArchive& Archive);

    FGuid               Guid;
    FMaterialInfo       Info;
    TAssetRef<FTexture> Textures[EMaterialTextureSlot::Count];

    /** @brief Reserved until emissive and vertex colours render (R0): read and written, but not used yet */
    FFloatColor         EmissiveColor        = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    float               EmissiveIntensity    = 1.0f;
    TAssetRef<FTexture> EmissiveTexture;
    bool                bMultiplyVertexColor = false;
};

struct ENGINE_API MaterialAssets
{
    static constexpr int32 CurrentVersion = 1;

    /** @brief Write Material in the text format, with defaults left out and its textures listed as Dependencies */
    NODISCARD static FJsonValue ToJson(const FMaterialAssetData& Material);

    /** @brief Read a material, failing with every error and its field path, such as "Routes.Roughness.Slot" */
    static bool FromJson(const FJsonValue& Value, FMaterialAssetData& OutMaterial, FString& OutError);

    static bool SaveToFile(const FString& Filename, const FMaterialAssetData& Material, FString& OutError);
    static bool LoadFromFile(const FString& Filename, FMaterialAssetData& OutMaterial, FString& OutError);

    /**
     * @brief Load a .dxrmaterial into an FMaterial, loading its textures through the asset loader, as the loader FAssetLoader
     * calls for materials. A texture that can't be loaded is logged and left out, and the base colour falls back to the engine's.
     */
    static TSharedPtr<FMaterial> Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError);
};
