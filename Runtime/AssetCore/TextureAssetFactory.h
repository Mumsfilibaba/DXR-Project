#pragma once
#include "AssetCore/AssetFactory.h"
#include "AssetCore/TextureAsset.h"
#include "Core/Containers/SharedPtr.h"

struct ITextureBlockCompressor
{
    virtual ~ITextureBlockCompressor() = default;

    /**
     * @brief Encode the mips of a texture into the 4x4 blocks of Format: BC4, BC5, BC6H or BC7
     * @param SourceFormat R8_UNorm for BC4, R8G8B8A8_UNorm for BC5 and BC7, whose sRGB values are encoded as they are, and
     * R16G16B16A16_Float for BC6H
     * @param OutMips Receives each mip's dimensions and its rows of blocks
     */
    virtual bool Compress(ETextureFormat SourceFormat, const TArray<FTextureMip>& Mips, ETextureFormat Format, TArray<FTextureMip>& OutMips, FString& OutError) = 0;
};

struct FTextureImportSettings
{
    ETextureUsage Usage = ETextureUsage::Color;

    /** @brief Store the colour as sRGB; only Color and Mask textures can be */
    bool bSRGB = true;

    /** @brief Compress to the usage's BCn format, when a compressor is set and the size is a multiple of 4 */
    bool bCompress = true;

    /** @brief The largest width or height to keep, halving the image until it fits, or 0 for the source's size */
    int32 MaxResolution = 0;

    bool bGenerateMips = true;
};

struct FTextureBuildJob
{
    FString SourcePath;

    /** @brief The settings asked for, a JSON object; empty for the defaults, which depend on the source */
    FJsonValue Settings;

    /** @brief Why it failed, naming the source, or empty when it succeeded */
    FString Error;

    /** @brief The DATA payload */
    TArray<uint8> Data;

    /** @brief The PNG thumbnail, at most ThumbnailSize on its longest side */
    TArray<uint8> Thumbnail;

    /** @brief The settings that were used, with the defaults filled in */
    FJsonValue UsedSettings;
};

class ASSETCORE_API FTextureAssetFactory final : public IAssetFactory
{
public:
    static constexpr int32 ThumbnailSize = 256;

    /**
     * @return Returns the usage to default to: HDR for a float image, Normal, Mask or Single for a name ending in a suffix
     * such as "_normal", "_orm" or "_roughness", Single for one channel, and Color otherwise
     */
    NODISCARD static ETextureUsage GuessUsage(const FString& SourcePath, int32 NumChannels, bool bHDR);

    /** @brief Read the settings of an import, taking Usage from DefaultUsage and sRGB from the usage when they are left out */
    static bool ReadSettings(const FJsonValue& Value, ETextureUsage DefaultUsage, FTextureImportSettings& OutSettings, FString& OutError);

    NODISCARD static FJsonValue WriteSettings(const FTextureImportSettings& Settings);

    /** @param InCompressor Encodes compressed textures; without one, every texture is stored uncompressed */
    explicit FTextureAssetFactory(const TSharedPtr<ITextureBlockCompressor>& InCompressor = nullptr);
    ~FTextureAssetFactory();

    virtual const CHAR_T* GetName() const override;
    virtual TArray<FString> GetExtensions() const override;
    virtual TArray<EAssetType> GetAssetTypes() const override;
    virtual bool Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError) override;

    /**
     * @brief Build the payloads and thumbnails of several textures. Decoding, resizing, mips and thumbnails run in parallel;
     * compression runs on the calling thread, one texture at a time.
     * @return Returns true if every job succeeded, and otherwise each failed job says why in its Error
     */
    bool BuildTextures(TArray<FTextureBuildJob>& Jobs) const;

private:
    TSharedPtr<ITextureBlockCompressor> Compressor;
};
