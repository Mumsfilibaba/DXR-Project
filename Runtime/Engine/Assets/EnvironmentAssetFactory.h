#pragma once
#include "AssetCore/AssetFactory.h"
#include "AssetCore/AssetRegistry.h"
#include "AssetCore/EnvironmentAsset.h"
#include "AssetCore/TextureAssetFactory.h"
#include "Engine/Resources/Environment.h"

struct FEnvironmentImportSettings
{
    /** @brief The width of each face of the sky cube */
    int32 SkySize = 1024;

    /** @brief Compress the cubes to BC6H */
    bool bCompress = true;
};

class ENGINE_API FEnvironmentAssetFactory final : public IAssetFactory
{
public:

    /** @brief The version of the filters a bake uses; raising it rebuilds environments baked before, as they load */
    static constexpr uint32 FilterVersion = 1;

    static constexpr int32 SpecularSize    = 256;
    static constexpr int32 NumSpecularMips = 5;
    static constexpr int32 DiffuseSize     = 32;

    static bool ReadSettings(const FJsonValue& Value, FEnvironmentImportSettings& OutSettings, FString& OutError);

    NODISCARD static FJsonValue WriteSettings(const FEnvironmentImportSettings& Settings);

    /** @brief Load an environment asset into an FEnvironment, as the loader FAssetLoader calls for environments, rebuilding it first when its bake is stale */
    static TSharedPtr<FEnvironment> Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError);

    /**
     * @brief Bake an environment on the GPU from a panorama already loaded as a texture, for one made in code rather than imported, such
     * as the samples' until they are assets. It has no asset identity.
     */
    static TSharedPtr<FEnvironment> CreateFromPanorama(FRHITexture* Panorama, int32 SkySize, FString& OutError);

    /** @brief Load a panorama file and bake an environment from it with CreateFromPanorama */
    static TSharedPtr<FEnvironment> CreateFromPanoramaFile(const FString& Filename, int32 SkySize, FString& OutError);

public:

    /**
     * @param InTextureFactory Decodes the panorama and makes the thumbnail
     * @param InCompressor Encodes the cubes in BC6H; without one they are stored as half floats
     */
    FEnvironmentAssetFactory(const TSharedPtr<FTextureAssetFactory>& InTextureFactory, const TSharedPtr<ITextureBlockCompressor>& InCompressor);
    ~FEnvironmentAssetFactory();

    virtual const CHAR_T* GetName() const override;
    virtual TArray<FString> GetExtensions() const override;
    virtual TArray<EAssetType> GetAssetTypes() const override;
    virtual bool Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError) override;

private:
    TSharedPtr<FTextureAssetFactory>    TextureFactory;
    TSharedPtr<ITextureBlockCompressor> Compressor;
};
