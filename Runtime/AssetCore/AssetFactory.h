#pragma once
#include "AssetCore/AssetType.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Json/JsonValue.h"
#include "Core/Misc/Guid.h"
#include "Core/Templates/Utility/NonCopyable.h"

struct FImportedAsset
{
    /** @brief The file name without its extension, which names the .dxrasset in the destination folder */
    FString Name;

    EAssetType Type = EAssetType::Unknown;

    /** @brief From FAssetImportContext::GetGuid, which the importer also fills in when it is left unset */
    FGuid Guid;

    /** @brief The payload, written as the DATA chunk */
    TArray<uint8> Data;

    /** @brief A PNG thumbnail, written as the THMB chunk, or empty for none */
    TArray<uint8> Thumbnail;

    /** @brief Every asset it references, which the cook follows */
    TArray<FGuid> Dependencies;
};

class ASSETCORE_API FAssetImportContext : public FNonCopyable
{
public:
    FAssetImportContext(const FString& InSourcePath, const FString& InDestinationDirectory, const FJsonValue& InSettings, const FString& InImportName);
    ~FAssetImportContext();

    /**
     * @return Returns the GUID for the asset named Name: the GUID of the .dxrasset already in the destination folder when
     * reimporting or replacing, so references to it stay valid, and otherwise a new one. The same name always gives the
     * same GUID within an import, so one asset can reference another before either is written.
     */
    FGuid GetGuid(const FString& Name);


    /**
     * @return Returns the base name to name the imported assets after: the source's file name on a first import, and the
     * name recorded then on a reimport, so a renamed source still updates the same assets
     */
    FORCEINLINE const FString& GetImportName() const
    {
        return ImportName;
    }

    /** @return Returns the source file on disk, which is the copy in ContentSources when one was made */
    FORCEINLINE const FString& GetSourcePath() const
    {
        return SourcePath;
    }

    /** @return Returns the settings to import with, a JSON object that is empty for the factory's defaults */
    FORCEINLINE const FJsonValue& GetSettings() const
    {
        return Settings;
    }

private:
    FString              SourcePath;
    FString              DestinationDirectory;
    FJsonValue           Settings;
    FString              ImportName;
    TMap<FString, FGuid> Guids;
};

struct IAssetFactory
{
    virtual ~IAssetFactory() = default;

    /** @return Returns the factory's name, recorded in META and used in messages, such as "Texture" */
    virtual const CHAR_T* GetName() const = 0;

    /** @return Returns the source extensions it imports, with the dot, such as ".png" */
    virtual TArray<FString> GetExtensions() const = 0;

    /** @return Returns the asset types an import can produce */
    virtual TArray<EAssetType> GetAssetTypes() const = 0;

    /** @brief List the other files an import reads, such as an OBJ's MTL and textures, so they are copied to ContentSources too */
    virtual void GetCompanionFiles(const FString& /*SourcePath*/, TArray<FString>& /*OutFiles*/) const
    {
    }

    /**
     * @brief Import the source into one or more assets
     * @param OutSettings Receives the settings that were used, with defaults filled in, which META records so a reimport
     * gives the same result. Left null, the given settings are recorded.
     * @return Returns false and says why in OutError if the source couldn't be imported
     */
    virtual bool Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError) = 0;
};
