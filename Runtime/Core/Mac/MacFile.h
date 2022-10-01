#pragma once
#include "Mac.h"

#include "Core/Generic/GenericFile.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacFileHandle

class CORE_API FMacFileHandle 
    : public IFileHandle
{
public:
    FMacFileHandle(HANDLE InFileHandle);
    ~FMacFileHandle() = default;

    virtual bool SeekFromStart(int64 InOffset) override final;
    virtual bool SeekFromCurrent(int64 InOffset) override final;
    virtual bool SeekFromEnd(int64 InOffset);

    virtual int64 Size() const override final;

    virtual int64 Tell() const override final;

    virtual int32 Read(uint8* Buffer, uint32 BufferSize) override final;
    virtual int32 Write(uint8* Buffer, uint32 BufferSize) override final;

    virtual bool Truncate(int64 NewSize) override final;

    virtual bool IsValid() const override final;

    virtual void Close() override final;

private:
    void UpdateFileSize()
    {
        Check(IsValid());

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

    HANDLE FileHandle;
    int64  FilePointer;
    int64  FileSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacFile

struct CORE_API FMacFile 
    : public FGenericFile
{
    static IFileHandle* OpenForRead(const FString& Filename)
    {
        return nullptr;
    }

    static IFileHandle* OpenForWrite(const FString& Filename)
    {
        return nullptr;
    }
};