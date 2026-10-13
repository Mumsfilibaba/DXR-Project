#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

enum class ETextureUsage : uint8
{
    /** @brief Colour, such as base colour: sRGB unless asked otherwise, and compressed to BC7 */
    Color = 0,

    /** @brief A tangent-space normal map: mips are renormalized, and it compresses to BC5, which keeps X and Y */
    Normal = 1,

    /** @brief Masks packed into the channels, such as occlusion, roughness and metalness: linear, and compressed to BC7 */
    Mask = 2,

    /** @brief One channel, such as roughness or height: linear, and compressed to BC4 */
    Single = 3,

    /** @brief High dynamic range colour in floats, compressed to BC6H */
    HDR = 4,
};

enum class ETextureFormat : uint8
{
    Unknown            = 0,
    R8_UNorm           = 1,
    R8G8B8A8_UNorm     = 2,
    R16G16B16A16_Float = 3,
    BC4_UNorm          = 4,
    BC5_UNorm          = 5,
    BC6H_UF16          = 6,
    BC7_UNorm          = 7,
};

struct FTextureMip
{
    int32 Width  = 0;
    int32 Height = 0;

    /** @brief Rows of pixels, or of 4x4 blocks in a block-compressed format, tightly packed with the top row first */
    TArray<uint8> Data;
};

struct FTextureAssetData
{
    ETextureFormat Format = ETextureFormat::Unknown;
    ETextureUsage  Usage  = ETextureUsage::Color;

    /** @brief The colour is stored as sRGB, which sampling turns linear */
    bool bSRGB = false;

    /** @brief The largest first, each half the size of the one before */
    TArray<FTextureMip> Mips;
};

struct ASSETCORE_API TextureAssets
{
    /** @brief A block-compressed format stores 4x4 pixels per block */
    static constexpr int32 BlockSize = 4;

    /** @return Returns the usage's name, as import settings write it, such as "Normal" */
    NODISCARD static const CHAR_T* GetUsageName(ETextureUsage Usage);

    /** @return Returns false when no usage is called Name */
    NODISCARD static bool FindUsageByName(const FString& Name, ETextureUsage& OutUsage);

    NODISCARD static const CHAR_T* GetFormatName(ETextureFormat Format);

    NODISCARD static bool IsBlockCompressed(ETextureFormat Format);

    /** @return Returns the bytes of a pixel, or of a 4x4 block in a block-compressed format, or 0 for Unknown */
    NODISCARD static int32 GetElementSize(ETextureFormat Format);

    /** @return Returns the bytes a mip of Format and these dimensions takes, tightly packed */
    NODISCARD static int64 GetMipSize(ETextureFormat Format, int32 Width, int32 Height);

    /** @return Returns the bytes of a row of pixels, or of a row of blocks in a block-compressed format */
    NODISCARD static int64 GetRowPitch(ETextureFormat Format, int32 Width);

    /** @return Returns the rows of pixels a mip has, or its rows of blocks in a block-compressed format */
    NODISCARD static int32 GetNumRows(ETextureFormat Format, int32 Height);

    /** @brief Write Texture as the DATA payload of a texture asset */
    static void Write(const FTextureAssetData& Texture, TArray<uint8>& OutPayload);

    /** @brief Read a texture asset's DATA payload, checking that each mip has the size its format and dimensions give */
    static bool Read(const TArray<uint8>& Payload, FTextureAssetData& OutTexture, FString& OutError);
};
