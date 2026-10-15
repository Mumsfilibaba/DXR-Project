#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Spinlock.h"
#include "RendererCore/RayTracing/AccelerationStructureCache.h"

class FRAMAccelerationStructureCacheBackend : public IAccelerationStructureCache
{
public:
    FRAMAccelerationStructureCacheBackend()           = default;
    ~FRAMAccelerationStructureCacheBackend() override = default;

    bool Load(const FStringView& Key, TArray<uint8>& OutBytes) override
    {
        TScopedLock Lock(EntriesLock);
        if (const TArray<uint8>* Existing = Entries.Find(FString(Key)))
        {
            OutBytes = *Existing;
            return !OutBytes.IsEmpty();
        }

        return false;
    }

    void Store(const FStringView& Key, const TArray<uint8>& Bytes) override
    {
        TScopedLock Lock(EntriesLock);
        Entries.Add(FString(Key), Bytes);
    }

    void Clear()
    {
        TScopedLock Lock(EntriesLock);
        Entries.Clear();
    }

private:
    TMap<FString, TArray<uint8>> Entries;
    FSpinLock                    EntriesLock;
};

class RENDERERCORE_API FDiskAccelerationStructureCacheBackend : public IAccelerationStructureCache
{
public:
    static constexpr const CHAR_T* FileExtension = TEXT(".rtas");

    /**
     * @param InDirectory The folder the entries are files in, created if missing
     * @param InMaxSizeInBytes The size the entries may take together, zero for no limit
     */
    FDiskAccelerationStructureCacheBackend(const FString& InDirectory, uint64 InMaxSizeInBytes);
    virtual ~FDiskAccelerationStructureCacheBackend();

    /** @brief Read the entry and mark it used now, so eviction keeps it longest */
    virtual bool Load(const FStringView& Key, TArray<uint8>& OutBytes) override final;

    /** @brief Write the entry, then evict the least recently used entries past the size limit */
    virtual void Store(const FStringView& Key, const TArray<uint8>& Bytes) override final;

    /** @brief Delete every entry ShouldRemove returns true for */
    void RemoveEntries(TFunction<bool(const FString& Key)> ShouldRemove);

    /** @brief Set the size the entries may take together, zero for no limit, and evict down to it */
    void SetMaxSize(uint64 InMaxSizeInBytes);

    FORCEINLINE const FString& GetDirectory() const
    {
        return Directory;
    }

    FORCEINLINE uint64 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

    FORCEINLINE int32 GetNumEntries() const
    {
        return Entries.Size();
    }

private:
    struct FEntry
    {
        FString Key;
        uint64  SizeInBytes  = 0;
        int64   LastUsedTime = 0;
    };

    void ScanDirectory();
    int32 EvictToMaxSize();
    void LogEviction(int32 NumEvicted, uint64 EvictedSizeInBytes) const;
    void RemoveEntryAt(int32 Index);
    int32 FindEntry(const FStringView& Key) const;
    FString CreateFilePath(const FStringView& Key) const;

    FString        Directory;
    TArray<FEntry> Entries;
    uint64         SizeInBytes;
    uint64         MaxSizeInBytes;
};
