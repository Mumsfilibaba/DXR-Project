#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
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

class FDiskAccelerationStructureCacheBackend : public IAccelerationStructureCache
{
public:
    explicit FDiskAccelerationStructureCacheBackend(const FString& InDirectory)
        : Directory(InDirectory)
    {
        // Ensure <AssetDir>/RTASCache (and any missing parents) exists so Store can persist entries
        File::CreateDirectoryTree(Directory);
    }

    ~FDiskAccelerationStructureCacheBackend() override = default;

    bool Load(const FStringView& Key, TArray<uint8>& OutBytes) override
    {
        const FString FilePath = CreateFilePath(Key);

        // A cache miss is expected - skip the open attempt (and its logging) when the file is absent
        if (!FPlatformFile::IsFile(*FilePath))
        {
            return false;
        }

        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(FilePath);
        if (!FileHandle)
        {
            return false;
        }

        if (!File::ReadFile(FileHandle.Get(), OutBytes))
        {
            return false;
        }

        return !OutBytes.IsEmpty();
    }

    void Store(const FStringView& Key, const TArray<uint8>& Bytes) override
    {
        if (Bytes.IsEmpty())
        {
            return;
        }

        const FString FilePath = CreateFilePath(Key);

        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(FilePath);
        if (!FileHandle)
        {
            return;
        }

        FileHandle->Write(Bytes.Data(), static_cast<uint32>(Bytes.SizeInBytes()));
    }

private:
    NODISCARD FString CreateFilePath(const FStringView& Key) const
    {
        return Directory + '/' + FString(Key) + ".rtas";
    }

    FString Directory;
};
