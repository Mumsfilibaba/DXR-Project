#pragma once
#include "Windows.h"

#include "Core/Generic/GenericFile.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsFileHandle

class CORE_API FWindowsFileHandle 
    : public IFileHandle
{
public:
    FWindowsFileHandle(HANDLE InFileHandle);
    ~FWindowsFileHandle() = default;

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
// FWindowsFile

struct CORE_API FWindowsFile 
    : public FGenericFile
{
    static FORCEINLINE IFileHandle* OpenForRead(const FString& Filename)
    {
        ::SetLastError(S_OK);

        HANDLE NewHandle = CreateFileA(
            Filename.GetCString(),
            GENERIC_READ,
            0,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0);

        if (NewHandle == INVALID_HANDLE_VALUE)
        {
            const auto LastError = GetLastError();
            return nullptr;
        }

        return dbg_new FWindowsFileHandle(NewHandle);
    }

    static FORCEINLINE IFileHandle* OpenForWrite(const FString& Filename)
    {
        ::SetLastError(S_OK);

        HANDLE NewHandle = ::CreateFileA(
            Filename.GetCString(),
            GENERIC_WRITE,
            0,
            0,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            0);

        if (NewHandle == INVALID_HANDLE_VALUE)
        {
            const auto LastError = ::GetLastError();
            return nullptr;
        }

        return dbg_new FWindowsFileHandle(NewHandle);
    }
};