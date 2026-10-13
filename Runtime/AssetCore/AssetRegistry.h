#pragma once
#include "AssetCore/AssetType.h"
#include "AssetCore/ContentMounts.h"
#include "Core/Containers/Map.h"
#include "Core/Delegates/Event.h"
#include "Core/Misc/Guid.h"
#include "Core/Templates/Utility/NonCopyable.h"

struct FAssetData
{
    FGuid      Guid;
    EAssetType Type = EAssetType::Unknown;

    /** @brief The virtual path, such as "/Project/Textures/Brick.dxrasset" */
    FString    Path;

    /** @brief The file name without its extension */
    FString    Name;
};

class FAssetRegistry;

DECLARE_EVENT(FOnAssetAddedEvent, FAssetRegistry, const FAssetData& /*Asset*/);
DECLARE_EVENT(FOnAssetRemovedEvent, FAssetRegistry, const FAssetData& /*Asset*/);
DECLARE_EVENT(FOnAssetRenamedEvent, FAssetRegistry, const FAssetData& /*Asset*/, const FString& /*OldPath*/);

class ASSETCORE_API FAssetRegistry : public FNonCopyable
{
public:

    /** @brief The registry file the cook writes into the project's Content folder, so a packaged game doesn't scan */
    static constexpr const CHAR_T* PrecomputedFilename = TEXT("assetregistry.json");
    static constexpr int32         PrecomputedVersion  = 1;

    FAssetRegistry();
    ~FAssetRegistry();

    /** @brief Use InMounts from the next Rescan or LoadFromFile on */
    void SetMounts(const FContentMounts& InMounts);

    /**
     * @brief Index every asset under the mounts again, and broadcast what was added, removed or moved since the last time.
     * A file that can't be read, a text asset without a valid Guid or Type, and a second file with a GUID already seen are
     * left out, logged as warnings and listed in GetErrors. Of two files with one GUID, the first by path is kept.
     */
    void Rescan();

    /** @brief Write the index as JSON, sorted by path, for a packaged game to load instead of scanning */
    bool SaveToFile(const FString& Filename, FString& OutError) const;

    /**
     * @brief Replace the index with one SaveToFile wrote, and broadcast the differences like Rescan
     * @return Returns false and leaves the index alone if the file is missing, not valid, or written by a newer engine
     */
    bool LoadFromFile(const FString& Filename, FString& OutError);

    NODISCARD const FAssetData* FindByGuid(const FGuid& Guid) const;
    NODISCARD const FAssetData* FindByPath(const FString& VirtualPath) const;

    /** @return Returns every asset, sorted by path */
    NODISCARD TArray<FAssetData> GetAssets() const;

    FORCEINLINE const FContentMounts& GetMounts() const
    {
        return Mounts;
    }

    FORCEINLINE int32 GetNumAssets() const
    {
        return Assets.Size();
    }

    FORCEINLINE const TArray<FString>& GetErrors() const
    {
        return Errors;
    }

    FORCEINLINE FOnAssetAddedEvent& GetOnAssetAddedEvent()
    {
        return OnAssetAddedEvent;
    }

    FORCEINLINE FOnAssetRemovedEvent& GetOnAssetRemovedEvent()
    {
        return OnAssetRemovedEvent;
    }

    FORCEINLINE FOnAssetRenamedEvent& GetOnAssetRenamedEvent()
    {
        return OnAssetRenamedEvent;
    }

private:
    bool ReadAssetFile(const FString& DiskPath, const FString& VirtualPath, FAssetData& OutAsset);
    void AddError(const FString& Error);
    void ApplyIndex(TMap<FGuid, FAssetData>&& NewAssets);

    FContentMounts          Mounts;
    TMap<FGuid, FAssetData> Assets;
    TMap<FString, FGuid>    GuidsByPath;
    TArray<FString>         Errors;
    FOnAssetAddedEvent      OnAssetAddedEvent;
    FOnAssetRemovedEvent    OnAssetRemovedEvent;
    FOnAssetRenamedEvent    OnAssetRenamedEvent;
};
