#pragma once
#include "AssetCore/AssetType.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Json/JsonValue.h"
#include "Core/Misc/Guid.h"

struct FAssetMetadata
{
    FString Name;

    /**
     * @brief The base name the import named its assets after, the source's file name at the first import. A reimport keeps
     * it, so a renamed or moved source still updates the same assets.
     */
    FString ImportName;

    /** @brief Where the asset was imported from: "Project:" or "Engine:" and a path under that root, or an absolute path */
    FString SourceFile;

    /** @brief The source file's hash when it was imported, to tell whether it changed since (see FAssetImporter::ComputeSourceHash) */
    FString SourceHash;

    /** @brief The name of the factory that imported it */
    FString Factory;

    /** @brief The settings it was imported with, a JSON object, so a reimport gives the same result */
    FJsonValue ImportSettings;

    /** @brief Every asset it references. The cook packages what these lists reach, so they have to be exact. */
    TArray<FGuid> Dependencies;
};

struct ASSETCORE_API AssetMetadata
{
    NODISCARD static FJsonValue ToJson(const FAssetMetadata& Metadata);

    /** @return Returns false and says why if Value isn't an object, or a field has the wrong type */
    static bool FromJson(const FJsonValue& Value, FAssetMetadata& OutMetadata, FString& OutError);

    /** @brief Read the META chunk of a .dxrasset */
    static bool ReadFromFile(const FString& DiskPath, FAssetMetadata& OutMetadata, FString& OutError);

    /**
     * @brief Read the GUIDs an asset references: the Dependencies in a .dxrasset's META, or the top-level Dependencies field of a
     * text asset (.dxrmaterial, .dxrmodel, .dxrscene). An asset without the field references nothing.
     */
    static bool ReadDependencies(const FString& DiskPath, TArray<FGuid>& OutDependencies, FString& OutError);
};
