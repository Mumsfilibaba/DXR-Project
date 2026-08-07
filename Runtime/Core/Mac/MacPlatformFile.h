#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Generic/GenericPlatformFile.h"
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
    virtual bool IsValid() const override final;
    virtual void Close() override final;

private:
    struct FPendingWrite;

    void GarbageCollectCompleted();
    void FreePendingWrite(FPendingWrite* PendingWrite);

    int32                  FileDescriptor;
    int64                  WriteOffset;
    TArray<FPendingWrite*> PendingWrites;
    bool                   bHasWriteError;
};

struct CORE_API FMacPlatformFile final : public FGenericPlatformFile
{
    static IPlatformFile* OpenForRead(const String& Filename);
    static IPlatformFile* OpenForWrite(const String& Filename, bool bTruncate = true);
    static IPlatformAsyncFile* OpenForAsyncWrite(const String& Filename, bool bTruncate = true);
    static String GetCurrentWorkingDirectory();
    static const CHAR* GetExecutablePath();

    static FORCEINLINE bool IsDirectory(const CHAR* Path)
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

    static FORCEINLINE bool IsFile(const CHAR* Path)
    {
        struct stat FileStat;
        return ::stat(Path, &FileStat) == 0;
    }

    static FORCEINLINE bool CreateDirectory(const CHAR* Path)
    {
        // Success if the directory was created now or already existed.
        return (::mkdir(Path, 0755) == 0) || (errno == EEXIST);
    }

    static FORCEINLINE bool RemoveDirectory(const CHAR* Path)
    {
        // Success if the directory is gone now or was never there.
        return (::rmdir(Path) == 0) || (errno == ENOENT);
    }

    static FORCEINLINE bool DeleteFile(const CHAR* Path)
    {
        // Success if the file is gone now or was never there.
        return (::unlink(Path) == 0) || (errno == ENOENT);
    }

    static FORCEINLINE bool MoveFile(const CHAR* FromFilename, const CHAR* ToFilename)
    {
        // Rename replaces an existing destination.
        return ::rename(FromFilename, ToFilename) == 0;
    }

    static FORCEINLINE bool IsPathRelative(const CHAR* Filepath)
    {
        return Filepath && Filepath[0] != '/';
    }
};
