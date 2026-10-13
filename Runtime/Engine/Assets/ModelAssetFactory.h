#pragma once
#include "AssetCore/AssetFactory.h"
#include "AssetCore/TextureAssetFactory.h"
#include "Engine/Assets/IModelImporter.h"
#include "Engine/Assets/ImportScene.h"

enum class EModelImportMode : uint8
{
    /** @brief A mesh asset for each mesh the selected nodes place, in its own space, with the first such node's materials as its defaults */
    Separate = 0,

    /** @brief One mesh asset holding the selected nodes' meshes with their transforms applied, a submesh and material slot for each part */
    Combine = 1,
};

struct FModelImportSettings
{
    EModelImportMode Mode = EModelImportMode::Separate;

    /** @brief The paths of the nodes to import, such as "Building/Door", each with the nodes under it, or empty for every node */
    TArray<FString> Nodes;

    bool bForceLeftHanded     = true;
    bool bInvertAxisX         = false;
    bool bApplyScaleFactor    = false;
    bool bRecalculateTangents = false;

    /** @brief Compress the textures to the BCn format their use calls for */
    bool bCompressTextures = true;

    /** @brief The largest width or height a texture keeps, or 0 for its source's size */
    int32 MaxTextureResolution = 0;
};

class ENGINE_API FModelAssetFactory final : public IAssetFactory
{
public:
    static bool ReadSettings(const FJsonValue& Value, FModelImportSettings& OutSettings, FString& OutError);

    NODISCARD static FJsonValue WriteSettings(const FModelImportSettings& Settings);

    /** @return Returns the node's name after its parents', joined by '/', which the Nodes setting selects it by */
    NODISCARD static FString GetNodePath(const FImportScene& Scene, int32 NodeIndex);

    /** @return Returns Name with the characters a file name can't hold replaced by '_', or "Unnamed" for nothing left */
    NODISCARD static FString MakeFileName(const FString& Name);

    explicit FModelAssetFactory(const TSharedPtr<FTextureAssetFactory>& InTextureFactory);
    ~FModelAssetFactory();

    virtual const CHAR_T* GetName() const override;
    virtual TArray<FString> GetExtensions() const override;
    virtual TArray<EAssetType> GetAssetTypes() const override;
    virtual void GetCompanionFiles(const FString& SourcePath, TArray<FString>& OutFiles) const override;
    virtual bool Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError) override;

private:
    NODISCARD IModelImporter* FindImporter(const FString& SourcePath) const;

    TSharedPtr<FTextureAssetFactory>   TextureFactory;
    TArray<TSharedPtr<IModelImporter>> Importers;
};
