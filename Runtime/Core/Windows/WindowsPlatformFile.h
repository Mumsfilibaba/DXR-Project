#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/StringConversion.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"

class CORE_API FWindowsFileHandle : public IPlatformFile
{
public:
    FWindowsFileHandle(HANDLE InFileHandle);
    virtual ~FWindowsFileHandle();

    virtual bool SeekFromStart(int64 InOffset) override final;
    virtual bool SeekFromCurrent(int64 InOffset) override final;
    virtual bool SeekFromEnd(int64 InOffset) override final;;
    virtual int64 Size() const override final;
    virtual int64 Tell() const override final;
    virtual int32 Read(uint8* Dst, uint32 BytesToRead) override final;
    virtual int32 Write(const uint8* Src, uint32 BytesToWrite) override final;
    virtual bool Truncate(int64 NewSize) override final;
    virtual bool IsValid() const override final;
    virtual void Close() override final;

private:
    bool Seek(int64 InOffset, DWORD MoveMethod);
    void UpdateFileSize();

    HANDLE FileHandle;
    int64  FilePointer;
    int64  FileSize;
};

class CORE_API FWindowsAsyncFileHandle : public IPlatformAsyncFile
{
public:
    FWindowsAsyncFileHandle(HANDLE InFileHandle);
    virtual ~FWindowsAsyncFileHandle();

    virtual bool WriteAsync(const uint8* Src, uint32 BytesToWrite) override final;
    virtual void WaitForPendingWrites() override final;
    virtual bool HasPendingWrites() const override final;
    virtual bool HasWriteError() const override final;
    virtual bool IsValid() const override final;
    virtual void Close() override final;

private:
    struct FPendingWrite
    {
        OVERLAPPED Overlapped;
        HANDLE     CompletionEvent;
        uint8*     Buffer;
    };

    void GarbageCollectCompleted();
    void FreePendingWrite(FPendingWrite* PendingWrite);
    bool WriteBlockingAtOffset(const uint8* Src, uint32 BytesToWrite, int64 Offset);
    void CollectWriteResult(FPendingWrite* PendingWrite, bool bWait);
    void ReportWriteFailure(const CHAR* What, uint32 ErrorCode);

    HANDLE                   FileHandle;
    int64                    WriteOffset;
    TArray<FPendingWrite*>   PendingWrites;
    mutable FCriticalSection PendingWritesCS;
    bool                     bHasWriteError;
};

struct CORE_API FWindowsPlatformFile : public IPlatformFileSystem
{
    static IPlatformFile* OpenForRead(const String& Filename);
    static IPlatformFile* OpenForWrite(const String& Filename, bool bTruncate = true);
    static IPlatformAsyncFile* OpenForAsyncWrite(const String& Filename, bool bTruncate = true);
    static String GetCurrentWorkingDirectory();
    static const CHAR* GetExecutablePath();
    static String GetUserSettingsDirectory();

    static FORCEINLINE bool IsDirectory(const CHAR* Path)
    {
        const BOOL Result = ::PathIsDirectoryW(StringCast<WIDECHAR>(Path).Get());
        return Result == static_cast<BOOL>(FILE_ATTRIBUTE_DIRECTORY);
    }

    static FORCEINLINE bool IsFile(const CHAR* Path)
    {
        const DWORD Attributes = ::GetFileAttributesW(StringCast<WIDECHAR>(Path).Get());
        return (Attributes != INVALID_FILE_ATTRIBUTES) && ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
    }

    static FFileInfo GetFileInfo(const CHAR* Path);

    static FORCEINLINE bool CreateDirectory(const CHAR* Path)
    {
        // Success if the directory was created now or already existed.
        return ::CreateDirectoryW(StringCast<WIDECHAR>(Path).Get(), nullptr) || (::GetLastError() == ERROR_ALREADY_EXISTS);
    }

    static FORCEINLINE bool RemoveDirectory(const CHAR* Path)
    {
        // Success if the directory is gone now or was never there.
        return ::RemoveDirectoryW(StringCast<WIDECHAR>(Path).Get()) || (::GetLastError() == ERROR_FILE_NOT_FOUND) || (::GetLastError() == ERROR_PATH_NOT_FOUND);
    }

    static FORCEINLINE bool DeleteFile(const CHAR* Path)
    {
        // Success if the file is gone now or was never there.
        return ::DeleteFileW(StringCast<WIDECHAR>(Path).Get()) || (::GetLastError() == ERROR_FILE_NOT_FOUND) || (::GetLastError() == ERROR_PATH_NOT_FOUND);
    }

    static FORCEINLINE bool MoveFile(const CHAR* FromFilename, const CHAR* ToFilename)
    {
        // MOVEFILE_REPLACE_EXISTING is what makes this usable when swapping a file.
        return ::MoveFileExW(StringCast<WIDECHAR>(FromFilename).Get(), StringCast<WIDECHAR>(ToFilename).Get(), MOVEFILE_REPLACE_EXISTING) != FALSE;
    }

    static FORCEINLINE bool CopyFile(const CHAR* FromFilename, const CHAR* ToFilename, bool bReplaceExisting = false)
    {
        return ::CopyFileW(StringCast<WIDECHAR>(FromFilename).Get(), StringCast<WIDECHAR>(ToFilename).Get(), bReplaceExisting ? FALSE : TRUE) != FALSE;
    }

    static FORCEINLINE bool MoveDirectory(const CHAR* FromPath, const CHAR* ToPath)
    {
        // MOVEFILE_REPLACE_EXISTING does not work for directories, so an existing ToPath makes this fail
        return ::MoveFileExW(StringCast<WIDECHAR>(FromPath).Get(), StringCast<WIDECHAR>(ToPath).Get(), 0) != FALSE;
    }

    static FORCEINLINE bool IsPathRelative(const CHAR* Filepath)
    {
        const BOOL Result = ::PathIsRelativeW(StringCast<WIDECHAR>(Filepath).Get());
        return Result == TRUE;
    }

    static bool IterateDirectory(const CHAR* Path, TArray<FDirectoryEntry>& OutEntries);
};
