#pragma once
#include "AssetCore/AssetFactory.h"
#include "AssetCore/ContentMounts.h"
#include "Core/Containers/SharedPtr.h"

enum class EAssetImportError : uint8
{
    None,

    /** @brief The source file doesn't exist; a reimport can be given the file's new location */
    SourceMissing,

    /** @brief No factory imports the source's extension */
    NoFactory,

    /** @brief The destination isn't a folder under a mount, or the asset to reimport isn't under one */
    InvalidDestination,

    /** @brief An asset the import would write exists already, and replacing wasn't asked for */
    DestinationExists,

    /** @brief A file the copy to ContentSources would write exists with other content, or can't be placed there */
    CopyConflict,

    /** @brief The factory failed, or returned an asset it can't produce */
    ImportFailed,

    /** @brief A file couldn't be read, copied or written */
    WriteFailed,
};

struct FAssetImportOptions
{
    /** @brief The settings to import with, a JSON object; empty for the factory's defaults */
    FJsonValue Settings;

    /**
     * @brief Copy the source and its companion files into <Root>/ContentSources/<the destination's folder> first, and import
     * the copy. A file there with other content is never overwritten.
     */
    bool bCopyToContentSources = false;

    /** @brief Replace assets that exist already, keeping their GUIDs so references to them stay valid */
    bool bReplaceExisting = false;

    /** @brief Also replace text assets, such as materials, that were edited since they were imported; otherwise they are kept */
    bool bOverwriteEditedAssets = false;
};

struct FAssetImportResult
{
    EAssetImportError Error = EAssetImportError::None;

    /** @brief Why it failed, naming the files involved */
    FString Message;

    /** @brief The virtual paths of the assets written */
    TArray<FString> WrittenAssets;

    /** @brief The virtual paths of text assets left as they were, because they were edited since they were imported */
    TArray<FString> KeptAssets;

    /** @brief What the factory left out or worked around, such as a texture it couldn't read */
    TArray<FString> Warnings;
};

class ASSETCORE_API FAssetImporter : public FNonCopyable
{
public:
    static constexpr const CHAR_T* ContentSourcesFolder = TEXT("ContentSources");

    /** @brief The member an imported text asset records the hash of the rest of its JSON in, to tell whether it was edited since */
    static constexpr const CHAR_T* ImportHashMember = TEXT("ImportHash");

    /** @brief Hash a file for SourceHash: 64-bit FNV-1a over its bytes, as 16 lowercase hex digits */
    static bool ComputeSourceHash(const FString& DiskPath, FString& OutHash);

    /** @return Returns the 64-bit FNV-1a hash of a JSON object as it is written, without its ImportHash member, as 16 lowercase hex digits */
    NODISCARD static FString ComputeImportHash(const FJsonValue& Document);

    /** @return Returns true if the text asset at DiskPath was edited since it was imported, or wasn't imported: its ImportHash is missing or doesn't match */
    NODISCARD static bool IsEditedTextAsset(const FString& DiskPath);

    explicit FAssetImporter(const FContentMounts& InMounts);
    ~FAssetImporter();

    /** @brief Import Factory's extensions with it from now on, ahead of factories registered before it */
    void RegisterFactory(const TSharedPtr<IAssetFactory>& Factory);
    void UnregisterFactory(const TSharedPtr<IAssetFactory>& Factory);

    /**
     * @brief Import a source file into the virtual folder DestinationFolder, such as "/Project/Textures"
     * @return Returns the assets written, or why nothing was: nothing is copied or written unless the whole import can be
     */
    FAssetImportResult ImportFile(const FString& SourcePath, const FString& DestinationFolder, const FAssetImportOptions& Options = FAssetImportOptions());

    /**
     * @brief Import an asset again from the source and with the settings its META records, keeping its GUID, along with every
     * other asset of the same import. Text assets edited since they were imported are kept unless bOverwriteEditedAssets.
     * @param LocatedSourcePath Where the source is now, when its recorded path no longer exists; recorded from then on
     * @return Returns EAssetImportError::SourceMissing, naming the recorded path, when the source can't be found
     */
    FAssetImportResult Reimport(const FString& AssetPath, const FString& LocatedSourcePath = FString(), bool bOverwriteEditedAssets = false);

    /** @return Returns the factory that imports SourcePath's extension, or nullptr */
    NODISCARD IAssetFactory* FindFactory(const FString& SourcePath) const;

    /** @return Returns how a source path is recorded: relative to the deepest mount root holding it with its prefix, or absolute */
    NODISCARD FString MakeSourcePath(const FString& DiskPath) const;

    /** @return Returns the file a recorded source path names */
    NODISCARD FString ResolveSourcePath(const FString& SourcePath) const;

private:
    FAssetImportResult CopyToContentSources(IAssetFactory& Factory, const FString& SourcePath, const FString& DestinationFolder, FString& OutCopiedSource) const;
    FAssetImportResult RunImport(IAssetFactory& Factory, const FString& SourcePath, const FString& DestinationFolder, const FJsonValue& Settings, const FString& ImportName, bool bReplaceExisting, bool bOverwriteEditedAssets) const;

    FContentMounts                    Mounts;
    TArray<TSharedPtr<IAssetFactory>> Factories;
};
