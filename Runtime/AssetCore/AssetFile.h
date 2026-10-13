#pragma once
#include "AssetCore/AssetType.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Json/JsonValue.h"
#include "Core/Misc/Guid.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
#include "Core/Templates/Utility/NonCopyable.h"

constexpr uint32 MakeAssetChunkId(const ANSICHAR (&Text)[5])
{
    return static_cast<uint32>(static_cast<uint8>(Text[0])) |
          (static_cast<uint32>(static_cast<uint8>(Text[1])) << 8) |
          (static_cast<uint32>(static_cast<uint8>(Text[2])) << 16) |
          (static_cast<uint32>(static_cast<uint8>(Text[3])) << 24);
}

struct AssetChunk
{
    /** @brief JSON metadata, such as the name, the source file and its hash, the import settings and the GUIDs depended on */
    static constexpr uint32 Metadata = MakeAssetChunkId("META");

    /** @brief A PNG thumbnail, which a cooked package leaves out */
    static constexpr uint32 Thumbnail = MakeAssetChunkId("THMB");

    /** @brief The payload, laid out by the asset type */
    static constexpr uint32 Data = MakeAssetChunkId("DATA");
};

struct FAssetFileHeader
{
    ANSICHAR   Magic[8];
    uint32     FileVersion;
    EAssetType Type;
    FGuid      Guid;
    uint32     NumChunks;

    /** @brief CRC of this header, with HeaderCRC zero, followed by the chunk table, so either is checked without the chunks */
    uint32     HeaderCRC;
};

struct FAssetChunkEntry
{
    uint32 Id;

    /** @brief CRC of the chunk's bytes, checked when the chunk is read */
    uint32 CRC;

    /** @brief Where the chunk starts, from the start of the file */
    uint64 Offset;
    uint64 Size;
};

static_assert(sizeof(FAssetFileHeader) == 40, "FAssetFileHeader is stored in files and must keep its layout");
static_assert(sizeof(FAssetChunkEntry) == 24, "FAssetChunkEntry is stored in files and must keep its layout");

struct ASSETCORE_API FAssetFile
{
    static constexpr const ANSICHAR* Magic          = "DXRASSET";
    static constexpr uint32          CurrentVersion = 1;
    static constexpr uint32          ChunkAlignment = 16;
    static constexpr uint32          MaxChunks      = 64;

    static constexpr const CHAR_T* FileExtension = TEXT(".dxrasset");

    /** @return Returns the chunk id as its four characters, for messages */
    NODISCARD static FString ChunkIdToString(uint32 Id);
};

class ASSETCORE_API FAssetFileWriter
{
public:
    FAssetFileWriter(const FGuid& InGuid, EAssetType InType);
    ~FAssetFileWriter();

    /** @brief Add a chunk, or replace the one with the same id. META is always written first, the rest in the order added. */
    void SetChunk(uint32 Id, TArray<uint8>&& Data);

    /**
     * @brief Add a chunk that refers to Data instead of copying it, or replace the one with the same id. Data has to stay
     * alive until the file is saved. This is how a chunk too large for an array, 2 GB or more, is written.
     */
    void SetChunkView(uint32 Id, const void* Data, uint64 Size);

    /** @brief Set the META chunk to Metadata, written as compact UTF-8 JSON */
    void SetMetadata(const FJsonValue& Metadata);

    /**
     * @brief Write the file a chunk at a time through a temporary file next to it, so a failed write leaves any existing
     * asset as it was
     * @return Returns false and says why in OutError if the file could not be written
     */
    bool SaveToFile(const FString& Filename, FString& OutError) const;

    FORCEINLINE const FGuid& GetGuid() const
    {
        return Guid;
    }

    FORCEINLINE EAssetType GetType() const
    {
        return Type;
    }

private:
    struct FChunk
    {
        uint32        Id;
        TArray<uint8> OwnedData;
        const uint8*  ViewData = nullptr;
        uint64        ViewSize = 0;
        bool          bIsView  = false;
    };

    FChunk& FindOrAddChunk(uint32 Id);

    FGuid          Guid;
    EAssetType     Type;
    TArray<FChunk> Chunks;
};

class ASSETCORE_API FAssetFileReader : public FNonCopyable
{
public:
    FAssetFileReader() = default;

    /**
     * @brief Open Filename and read its header and chunk table
     * @return Returns false and says why in OutError if the file is missing, not a .dxrasset, written by a newer engine,
     * truncated, or fails its header checksum
     */
    bool Open(const FString& Filename, FString& OutError);

    void Close();

    /** @return Returns the chunk's entry, or nullptr when the asset has no such chunk */
    NODISCARD const FAssetChunkEntry* FindChunk(uint32 Id) const;

    /**
     * @brief Read a whole chunk into an array, which holds less than 2 GB
     * @return Returns false and says why in OutError if there is no such chunk, it is too large, it can't be read, or it
     * fails its checksum
     */
    bool ReadChunk(uint32 Id, TArray<uint8>& OutData, FString& OutError);

    /**
     * @brief Read a whole chunk of any size into Destination, which DestinationSize says holds at least the chunk's size
     * @return Returns false and says why in OutError if there is no such chunk, Destination is too small, the chunk can't be
     * read, or it fails its checksum
     */
    bool ReadChunk(uint32 Id, void* Destination, uint64 DestinationSize, FString& OutError);

    /**
     * @brief Read Size bytes starting Offset bytes into a chunk, such as one mip of a texture. The chunk's CRC covers the
     * whole chunk, so it isn't checked.
     * @return Returns false and says why in OutError if there is no such chunk, the range runs past its end, or it can't be read
     */
    bool ReadChunkRange(uint32 Id, uint64 Offset, void* Destination, uint64 Size, FString& OutError);

    /** @return Returns false and says why in OutError if the META chunk can't be read or isn't valid JSON */
    bool ReadMetadata(FJsonValue& OutMetadata, FString& OutError);

    FORCEINLINE bool HasChunk(uint32 Id) const
    {
        return FindChunk(Id) != nullptr;
    }

    FORCEINLINE bool IsOpen() const
    {
        return File.IsValid();
    }

    FORCEINLINE const FString& GetFilename() const
    {
        return Filename;
    }

    FORCEINLINE const FGuid& GetGuid() const
    {
        return Header.Guid;
    }

    FORCEINLINE EAssetType GetType() const
    {
        return Header.Type;
    }

    FORCEINLINE const TArray<FAssetChunkEntry>& GetChunks() const
    {
        return Chunks;
    }

private:
    const FAssetChunkEntry* FindChunkForRead(uint32 Id, FString& OutError) const;
    bool ReadBytesAt(uint64 Offset, void* Destination, uint64 Size);

    FString                  Filename;
    TFileRef<IPlatformFile>  File;
    FAssetFileHeader         Header{};
    TArray<FAssetChunkEntry> Chunks;
};
