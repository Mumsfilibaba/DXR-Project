#pragma once
#include "AssetCore/TextureAsset.h"

struct FEnvironmentCube
{
    ETextureFormat      Format = ETextureFormat::Unknown;
    TArray<FTextureMip> Mips;
};

struct FEnvironmentAssetData
{
    /** @brief The version of the filters that baked it, so a bake by older filters can be rebuilt */
    uint32 FilterVersion = 0;

    /** @brief The sky as the skybox draws it, with a full mip chain */
    FEnvironmentCube Sky;

    /** @brief The sky prefiltered for GGX specular reflections, one roughness level per mip */
    FEnvironmentCube Specular;

    /** @brief The sky's diffuse irradiance */
    FEnvironmentCube Diffuse;
};

struct ASSETCORE_API EnvironmentAssets
{
    static constexpr int32 NumCubeFaces = 6;

    /** @brief Write an environment asset's DATA payload */
    static void Write(const FEnvironmentAssetData& Environment, TArray<uint8>& OutPayload);

    /** @brief Read an environment asset's DATA payload, checking that each mip has six square faces of the size its format gives */
    static bool Read(const TArray<uint8>& Payload, FEnvironmentAssetData& OutEnvironment, FString& OutError);
};
