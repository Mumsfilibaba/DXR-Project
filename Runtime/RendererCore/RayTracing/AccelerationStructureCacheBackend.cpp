#include "Core/Filesystem/File.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformFile.h"
#include "RendererCore/RayTracing/AccelerationStructureCacheBackend.h"

FDiskAccelerationStructureCacheBackend::FDiskAccelerationStructureCacheBackend(const FString& InDirectory, uint64 InMaxSizeInBytes)
    : Directory(InDirectory)
    , Entries()
    , SizeInBytes(0)
    , MaxSizeInBytes(InMaxSizeInBytes)
{
    File::CreateDirectoryTree(Directory);
    ScanDirectory();

    const uint64 SizeBefore = SizeInBytes;
    LogEviction(EvictToMaxSize(), SizeBefore - SizeInBytes);
}

FDiskAccelerationStructureCacheBackend::~FDiskAccelerationStructureCacheBackend() = default;

bool FDiskAccelerationStructureCacheBackend::Load(const FStringView& Key, TArray<uint8>& OutBytes)
{
    const int32 EntryIndex = FindEntry(Key);
    if (EntryIndex < 0)
    {
        return false;
    }

    const FString FilePath = CreateFilePath(Key);
    {
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(FilePath);
        if (!FileHandle || !File::ReadFile(FileHandle.Get(), OutBytes) || OutBytes.IsEmpty())
        {
            return false;
        }
    }

    {
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(FilePath, false);
        if (FileHandle)
        {
            FileHandle->Write(OutBytes.Data(), static_cast<uint32>(Math::Min<int32>(OutBytes.Size(), 4)));
        }
    }

    Entries[EntryIndex].LastUsedTime = FPlatformFile::GetFileInfo(*FilePath).ModifiedTime;
    return true;
}

void FDiskAccelerationStructureCacheBackend::Store(const FStringView& Key, const TArray<uint8>& Bytes)
{
    if (Bytes.IsEmpty())
    {
        return;
    }

    const FString FilePath = CreateFilePath(Key);
    const FString TempPath = FilePath + TEXT(".tmp");
    {
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(TempPath);
        if (!FileHandle)
        {
            return;
        }

        if (FileHandle->Write(Bytes.Data(), static_cast<uint32>(Bytes.SizeInBytes())) != Bytes.SizeInBytes())
        {
            FileHandle.Close();
            FPlatformFile::DeleteFile(*TempPath);
            return;
        }
    }

    const int32 ExistingIndex = FindEntry(Key);
    if (ExistingIndex >= 0)
    {
        RemoveEntryAt(ExistingIndex);
    }

    if (!FPlatformFile::MoveFile(*TempPath, *FilePath))
    {
        FPlatformFile::DeleteFile(*TempPath);
        return;
    }

    FEntry& Entry = Entries.Emplace();
    Entry.Key          = FString(Key);
    Entry.SizeInBytes  = Bytes.SizeInBytes();
    Entry.LastUsedTime = FPlatformFile::GetFileInfo(*FilePath).ModifiedTime;
    SizeInBytes += Entry.SizeInBytes;

    EvictToMaxSize();
}

void FDiskAccelerationStructureCacheBackend::RemoveEntries(TFunction<bool(const FString& Key)> ShouldRemove)
{
    for (int32 Index = Entries.Size() - 1; Index >= 0; --Index)
    {
        if (ShouldRemove(Entries[Index].Key))
        {
            RemoveEntryAt(Index);
        }
    }
}

void FDiskAccelerationStructureCacheBackend::SetMaxSize(uint64 InMaxSizeInBytes)
{
    MaxSizeInBytes = InMaxSizeInBytes;

    const uint64 SizeBefore = SizeInBytes;
    LogEviction(EvictToMaxSize(), SizeBefore - SizeInBytes);
}

void FDiskAccelerationStructureCacheBackend::ScanDirectory()
{
    Entries.Clear();
    SizeInBytes = 0;

    TArray<FDirectoryEntry> DirectoryEntries;
    if (!FPlatformFile::IterateDirectory(*Directory, DirectoryEntries))
    {
        return;
    }

    const FString Extension(FileExtension);
    for (const FDirectoryEntry& DirectoryEntry : DirectoryEntries)
    {
        if (DirectoryEntry.bIsDirectory)
        {
            continue;
        }

        const FString FilePath = Directory + '/' + DirectoryEntry.Name;
        if (DirectoryEntry.Name.EndsWith(TEXT(".tmp")))
        {
            FPlatformFile::DeleteFile(*FilePath);
            continue;
        }

        if (!DirectoryEntry.Name.EndsWith(*Extension))
        {
            continue;
        }

        const FFileInfo FileInfo = FPlatformFile::GetFileInfo(*FilePath);

        FEntry& Entry = Entries.Emplace();
        Entry.Key          = DirectoryEntry.Name.SubString(0, DirectoryEntry.Name.Length() - Extension.Length());
        Entry.SizeInBytes  = static_cast<uint64>(FileInfo.Size);
        Entry.LastUsedTime = FileInfo.ModifiedTime;
        SizeInBytes += Entry.SizeInBytes;
    }
}

int32 FDiskAccelerationStructureCacheBackend::EvictToMaxSize()
{
    if (MaxSizeInBytes == 0 || SizeInBytes <= MaxSizeInBytes)
    {
        return 0;
    }

    const int32 NumBefore = Entries.Size();
    while (SizeInBytes > MaxSizeInBytes && !Entries.IsEmpty())
    {
        int32 OldestIndex = 0;
        for (int32 Index = 1; Index < Entries.Size(); ++Index)
        {
            if (Entries[Index].LastUsedTime < Entries[OldestIndex].LastUsedTime)
            {
                OldestIndex = Index;
            }
        }

        RemoveEntryAt(OldestIndex);
    }

    return NumBefore - Entries.Size();
}

void FDiskAccelerationStructureCacheBackend::LogEviction(int32 NumEvicted, uint64 EvictedSizeInBytes) const
{
    if (NumEvicted > 0)
    {
        LOG_INFO("[FDiskAccelerationStructureCacheBackend]: Evicted %d entries (%.1f MB) from '%s' to stay under %.1f MB", NumEvicted,
            double(EvictedSizeInBytes) / (1024.0 * 1024.0), *Directory, double(MaxSizeInBytes) / (1024.0 * 1024.0));
    }
}

void FDiskAccelerationStructureCacheBackend::RemoveEntryAt(int32 Index)
{
    FPlatformFile::DeleteFile(*CreateFilePath(FStringView(Entries[Index].Key)));
    SizeInBytes -= Entries[Index].SizeInBytes;
    Entries.RemoveAt(Index);
}

int32 FDiskAccelerationStructureCacheBackend::FindEntry(const FStringView& Key) const
{
    for (int32 Index = 0; Index < Entries.Size(); ++Index)
    {
        if (FStringView(Entries[Index].Key) == Key)
        {
            return Index;
        }
    }

    return -1;
}

FString FDiskAccelerationStructureCacheBackend::CreateFilePath(const FStringView& Key) const
{
    return Directory + '/' + FString(Key) + FileExtension;
}
