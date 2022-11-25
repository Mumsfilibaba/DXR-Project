#pragma once
#include "Windows.h"

#include "Core/Generic/GenericFile.h"

class CORE_API FWindowsFileHandle 
    : public IFileHandle
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
    void UpdateFileSize()
    {
        if ((FileHandle != 0) && (FileHandle != INVALID_HANDLE_VALUE))
        {
            LARGE_INTEGER TempFileSize;
            if (!GetFileSizeEx(FileHandle, &TempFileSize))
            {
                FileSize = -1;
            }
            else
            {
                FileSize = static_cast<int64>(TempFileSize.QuadPart);
            }
        }

        CHECK(IsValid());
    }

    HANDLE FileHandle;
    int64  FilePointer;
    int64  FileSize;
};


struct CORE_API FWindowsFile 
    : public FGenericFile
{
    static IFileHandle* OpenForRead(const FString& Filename);

    static IFileHandle* OpenForWrite(const FString& Filename);

    static FString GetCurrentDirectory();

    static bool IsPathRelative(const CHAR* Filepath);
};