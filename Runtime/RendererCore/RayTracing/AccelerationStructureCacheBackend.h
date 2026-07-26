#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Generic/GenericPlatformFile.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Spinlock.h"
#include "RendererCore/RayTracing/AccelerationStructureCache.h"

class FRAMAccelerationStructureCacheBackend : public IAccelerationStructureCache
{
public:
    FRAMAccelerationStructureCacheBackend()           = default;
    ~FRAMAccelerationStructureCacheBackend() override = default;

    bool Load(const StringView& Key, TArray<uint8>& OutBytes) override
    {
        TScopedLock Lock(EntriesLock);
        if (const TArray<uint8>* Existing = Entries.Find(String(Key)))
        {
            OutBytes = *Existing;
            return !OutBytes.IsEmpty();
        }

        return false;
    }

    void Store(const StringView& Key, const TArray<uint8>& Bytes) override
    {
        TScopedLock Lock(EntriesLock);
        Entries.Add(String(Key), Bytes);
    }

    void Clear()
    {
        TScopedLock Lock(EntriesLock);
        Entries.Clear();
    }

private:
    TMap<String, TArray<uint8>> Entries;
    FSpinLock                   EntriesLock;
};

class FDiskAccelerationStructureCacheBackend : public IAccelerationStructureCache
{
public:
    explicit FDiskAccelerationStructureCacheBackend(const String& InDirectory)
        : Directory(InDirectory)
    {
        // Ensure <AssetDir>/RTASCache (and any missing parents) exists so Store can persist entries
        File::CreateDirectoryTree(Directory);
    }

    ~FDiskAccelerationStructureCacheBackend() override = default;

    bool Load(const StringView& Key, TArray<uint8>& OutBytes) override
    {
        const String FilePath = MakeFilePath(Key);

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

    void Store(const StringView& Key, const TArray<uint8>& Bytes) override
    {
        if (Bytes.IsEmpty())
        {
            return;
        }

        const String FilePath = MakeFilePath(Key);

        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(FilePath);
        if (!FileHandle)
        {
            return;
        }

        FileHandle->Write(Bytes.Data(), static_cast<uint32>(Bytes.SizeInBytes()));
    }

private:
    NODISCARD String MakeFilePath(const StringView& Key) const
    {
        return Directory + '/' + String(Key) + ".rtas";
    }

    String Directory;
};
