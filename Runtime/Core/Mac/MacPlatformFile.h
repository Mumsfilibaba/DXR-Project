#pragma once
#include "Core/Mac/Mac.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
#include <sys/stat.h>
#include <cerrno>
#include <cstdio>
#include <unistd.h>

class CORE_API FMacFileHandle : public IPlatformFile
{
    static constexpr int64 MaxReadWriteSize = 1024 * 1024;

public:
    FMacFileHandle(int32 InFileHandle, bool bInReadOnly);
    virtual ~FMacFileHandle();

    virtual bool SeekFromStart(int64 InOffset) override final;
    virtual bool SeekFromCurrent(int64 InOffset) override final;
    virtual bool SeekFromEnd(int64 InOffset) override final;
    virtual int64 Size() const override final;
    virtual int64 Tell() const override final;
    virtual int32 Read(uint8* Dst, uint32 BytesToRead) override final;
    virtual int32 Write(const uint8* Src, uint32 BytesToWrite) override final;
    virtual bool Truncate(int64 NewSize) override final;
    virtual bool IsValid() const override final;
    virtual void Close() override final;

private:
    int32 FileHandle;
    bool  bReadOnly;
};

class CORE_API FMacAsyncFileHandle : public IPlatformAsyncFile
{
public:
    FMacAsyncFileHandle(int32 InFileDescriptor);
    virtual ~FMacAsyncFileHandle();

    virtual bool WriteAsync(const uint8* Src, uint32 BytesToWrite) override final;
    virtual void WaitForPendingWrites() override final;
    virtual bool HasPendingWrites() const override final;
    virtual bool HasWriteError() const override final;
    virtual bool IsValid() const override final;
    virtual void Close() override final;

private:
    struct FPendingWrite;

    void GarbageCollectCompleted();
    void FreePendingWrite(FPendingWrite* PendingWrite);
    bool WriteBlockingAtOffset(const uint8* Src, uint32 BytesToWrite, int64 Offset);
    void ReportWriteFailure(const CHAR_T* What, int32 ErrorCode);

    int32                  FileDescriptor;
    int64                  WriteOffset;
    TArray<FPendingWrite*> PendingWrites;
    bool                   bHasWriteError;
};

struct CORE_API FMacPlatformFile final : public IPlatformFileSystem
{
    static IPlatformFile* OpenForRead(const FString& Filename);
    static IPlatformFile* OpenForWrite(const FString& Filename, bool bTruncate = true);
    static IPlatformAsyncFile* OpenForAsyncWrite(const FString& Filename, bool bTruncate = true);
    static FString GetCurrentWorkingDirectory();
    static const CHAR_T* GetExecutablePath();
    static FString GetUserSettingsDirectory();

    static FORCEINLINE bool IsDirectory(const CHAR_T* Path)
    {
        struct stat PathStat;
        if (::stat(Path, &PathStat) == 0) 
        {
            return S_ISDIR(PathStat.st_mode);
        }
        else
        {
            return false;
        }
    }

    static FORCEINLINE bool IsFile(const CHAR_T* Path)
    {
        struct stat FileStat;
        return (::stat(Path, &FileStat) == 0) && !S_ISDIR(FileStat.st_mode);
    }

    static FFileInfo GetFileInfo(const CHAR_T* Path);

    static FORCEINLINE bool CreateDirectory(const CHAR_T* Path)
    {
        // Success if the directory was created now or already existed.
        return (::mkdir(Path, 0755) == 0) || (errno == EEXIST);
    }

    static FORCEINLINE bool RemoveDirectory(const CHAR_T* Path)
    {
        // Success if the directory is gone now or was never there.
        return (::rmdir(Path) == 0) || (errno == ENOENT);
    }

    static FORCEINLINE bool DeleteFile(const CHAR_T* Path)
    {
        // Success if the file is gone now or was never there.
        return (::unlink(Path) == 0) || (errno == ENOENT);
    }

    static FORCEINLINE bool MoveFile(const CHAR_T* FromFilename, const CHAR_T* ToFilename)
    {
        // Rename replaces an existing destination.
        return ::rename(FromFilename, ToFilename) == 0;
    }

    static bool CopyFile(const CHAR_T* FromFilename, const CHAR_T* ToFilename, bool bReplaceExisting = false);

    static FORCEINLINE bool MoveDirectory(const CHAR_T* FromPath, const CHAR_T* ToPath)
    {
        struct stat PathStat;
        if (::stat(ToPath, &PathStat) == 0)
        {
            return false;
        }

        return ::rename(FromPath, ToPath) == 0;
    }

    static FORCEINLINE bool IsPathRelative(const CHAR_T* Filepath)
    {
        return Filepath && Filepath[0] != '/';
    }

    static bool IterateDirectory(const CHAR_T* Path, TArray<FDirectoryEntry>& OutEntries);
};
