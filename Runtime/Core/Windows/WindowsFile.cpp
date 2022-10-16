#include "WindowsFile.h"
#include "WindowsPlatformMisc.h"

#include "Core/Templates/NumericLimits.h"

FWindowsFileHandle::FWindowsFileHandle(HANDLE InFileHandle)
    : IFileHandle()
    , FileHandle(InFileHandle)
    , FileSize(-1)
    , FilePointer(0)
{
    UpdateFileSize();
}

bool FWindowsFileHandle::SeekFromStart(int64 InOffset)
{
    CHECK(IsValid());

    LARGE_INTEGER Offset;
    Offset.QuadPart = InOffset;
    return SetFilePointerEx(FileHandle, Offset, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
}

bool FWindowsFileHandle::SeekFromCurrent(int64 InOffset)
{
    CHECK(IsValid());

    LARGE_INTEGER Offset;
    Offset.QuadPart = InOffset;
    return SetFilePointerEx(FileHandle, Offset, nullptr, FILE_CURRENT) != INVALID_SET_FILE_POINTER;
}

bool FWindowsFileHandle::SeekFromEnd(int64 InOffset)
{
    CHECK(IsValid());

    LARGE_INTEGER Offset;
    Offset.QuadPart = InOffset;
    return SetFilePointerEx(FileHandle, Offset, nullptr, FILE_END) != INVALID_SET_FILE_POINTER;
}

int64 FWindowsFileHandle::Size() const
{
    CHECK(IsValid());
    return FileSize;
}

int64 FWindowsFileHandle::Tell() const
{
    CHECK(IsValid());
    return FilePointer;
}

int32 FWindowsFileHandle::Read(uint8* Dst, uint32 BytesToRead)
{
    CHECK(IsValid());

    ::SetLastError(0);

    int32 TotalRead = 0;
    while(BytesToRead)
    {
        const uint32 LocalBytesToRead = NMath::Min<uint32>(BytesToRead, TNumericLimits<uint32>::Max());

        DWORD NumRead = 0;
        if (!ReadFile(FileHandle, Dst, LocalBytesToRead, &NumRead, nullptr))
        {
            const int32 Error = ::GetLastError();
        
            // ERROR_IO_PENDING is not an error, however if the error is not that we report an error
            if (Error != ERROR_IO_PENDING)
            {
                FString ErrorString;

                FWindowsPlatformMisc::GetLastErrorString(ErrorString);
                LOG_ERROR("Failed to read file, Error '%d' Message '%s'", Error, ErrorString.GetCString());
                
                return -1;
            }
        }

        BytesToRead -= NumRead;
        Dst         += NumRead;
        TotalRead   += NumRead;

        FilePointer += NumRead;
        CHECK(FilePointer <= FileSize);
        
        // We may have reached end of file here
        if (LocalBytesToRead != NumRead)
        {
            break;
        }
    }

    return static_cast<int32>(TotalRead);
}

int32 FWindowsFileHandle::Write(const uint8* Src, uint32 BytesToWrite)
{
    CHECK(IsValid());

    DWORD NumWritten = 0;
    if (!WriteFile(FileHandle, Src, BytesToWrite, &NumWritten, nullptr))
    {
        const auto Error = GetLastError();

        // ERROR_IO_PENDING is not an error, however if the error is not that we report an error
        if (Error != ERROR_IO_PENDING)
        {
            return -1;
        }
    }

    FilePointer += NumWritten;
    return static_cast<int32_t>(NumWritten);
}

bool FWindowsFileHandle::Truncate(int64 NewSize)
{
    CHECK(IsValid());

    if (SeekFromStart(NewSize) && SetEndOfFile(FileHandle) != 0)
    {
        UpdateFileSize();
        CHECK(IsValid());
        return true;
    }

    return false;
}

bool FWindowsFileHandle::IsValid() const
{
    return (FileHandle != 0) && (FileHandle != INVALID_HANDLE_VALUE) && (FileSize != -1);
}

void FWindowsFileHandle::Close()
{
    if (IsValid())
    {
        CloseHandle(FileHandle);
    }
    
    FileHandle = INVALID_HANDLE_VALUE;
    FileSize   = -1;
    delete this;
}


FORCEINLINE IFileHandle* FWindowsFile::OpenForRead(const FString& Filename)
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
        MAYBE_UNUSED const auto LastError = ::GetLastError();
        return nullptr;
    }

    return dbg_new FWindowsFileHandle(NewHandle);
}

FORCEINLINE IFileHandle* FWindowsFile::OpenForWrite(const FString& Filename)
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
        MAYBE_UNUSED const auto LastError = ::GetLastError();
        return nullptr;
    }

    return dbg_new FWindowsFileHandle(NewHandle);
}