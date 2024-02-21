#pragma once
#include "Mac.h"
#include "Core/Generic/GenericPlatformFile.h"
#include <sys/stat.h>

class CORE_API FMacFileHandle : public IFileHandle
{
    static constexpr int64 MaxReadWriteSize = 1024 * 1024;

public:
    FMacFileHandle(int32 InFileHandle, bool bInReadOnly);
    virtual ~FMacFileHandle() = default;

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
    int32 FileHandle = -1;
    bool  bReadOnly;
};

struct CORE_API FMacPlatformFile final : public FGenericPlatformFile
{
    static IFileHandle* OpenForRead(const FString& Filename);
    static IFileHandle* OpenForWrite(const FString& Filename, bool bTruncate = true);

    static const CHAR* GetExecutablePath();
    static FString     GetCurrentWorkingDirectory();

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

    static FORCEINLINE bool IsPathRelative(const CHAR* Filepath)
    {
        return Filepath && Filepath[0] != '/';
    }
};
