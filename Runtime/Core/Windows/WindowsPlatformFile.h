#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Generic/GenericPlatformFile.h"

class CORE_API FWindowsFileHandle : public IPlatformFile
{
public:
    FWindowsFileHandle(HANDLE InFileHandle);
    virtual ~FWindowsFileHandle() = default;

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
    void UpdateFileSize();

    HANDLE FileHandle;
    int64  FilePointer;
    int64  FileSize;
};

class CORE_API FWindowsAsyncFileHandle : public IPlatformAsyncFile
{
public:
    FWindowsAsyncFileHandle(HANDLE InFileHandle);
    virtual ~FWindowsAsyncFileHandle() = default;

    virtual bool WriteAsync(const uint8* Src, uint32 BytesToWrite) override final;
    virtual void WaitForPendingWrites() override final;
    virtual bool HasPendingWrites() const override final;
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

    HANDLE                 FileHandle;
    int64                  WriteOffset;
    TArray<FPendingWrite*> PendingWrites;
};

struct CORE_API FWindowsPlatformFile : public FGenericPlatformFile
{
    static IPlatformFile* OpenForRead(const FString& Filename);
    static IPlatformFile* OpenForWrite(const FString& Filename, bool bTruncate = true);
    static IPlatformAsyncFile* OpenForAsyncWrite(const FString& Filename, bool bTruncate = true);
    static FString GetCurrentWorkingDirectory();
    static const CHAR* GetExecutablePath();

    static FORCEINLINE bool IsDirectory(const CHAR* Path)
    {
        const BOOL Result = ::PathIsDirectoryA(Path);
        return Result == static_cast<BOOL>(FILE_ATTRIBUTE_DIRECTORY);
    }

    static FORCEINLINE bool IsFile(const CHAR* Path)
    {
        const BOOL Result = ::PathFileExistsA(Path);
        return Result == TRUE;
    }

    static FORCEINLINE bool IsPathRelative(const CHAR* Filepath)
    {
        const BOOL Result = ::PathIsRelativeA(Filepath);
        return Result == TRUE;
    }
};
