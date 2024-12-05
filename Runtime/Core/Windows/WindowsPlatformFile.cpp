#include "WindowsPlatformFile.h"
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
        const uint32 LocalBytesToRead = FMath::Min<uint32>(BytesToRead, TNumericLimits<uint32>::Max());

        DWORD NumRead = 0;
        if (!ReadFile(FileHandle, Dst, LocalBytesToRead, &NumRead, nullptr))
        {
            const int32 Error = ::GetLastError();
        
            // ERROR_IO_PENDING is not an error, however if the error is not that we report an error
            if (Error != ERROR_IO_PENDING)
            {
                FString ErrorString;

                FWindowsPlatformMisc::GetLastErrorString(ErrorString);
                LOG_ERROR("Failed to read file, Error '%d' Message '%s'", Error, *ErrorString);
                
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


IFileHandle* FWindowsPlatformFile::OpenForRead(const FString& Filename)
{
    ::SetLastError(S_OK);

    HANDLE NewHandle = CreateFileA(
        *Filename,
        GENERIC_READ,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        FString ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);
        
        auto Position = ErrorString.FindLast("\r\n");
        if (Position != FString::InvalidIndex)
        {
            ErrorString.Remove(Position, 2);
        }

        LOG_ERROR("[FWindowsPlatformFile] Failed to open file. Error '%s'", *ErrorString);
        return nullptr;
    }

    return new FWindowsFileHandle(NewHandle);
}

IFileHandle* FWindowsPlatformFile::OpenForWrite(const FString& Filename, bool bTruncate)
{
    ::SetLastError(S_OK);

    HANDLE NewHandle = ::CreateFileA(
        *Filename,
        GENERIC_WRITE,
        0,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        FString ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);

        int32 Position = ErrorString.FindLast("\r\n");
        if (Position != FString::InvalidIndex)
        {
            ErrorString.Remove(Position, 2);
        }

        LOG_ERROR("[FWindowsPlatformFile] Failed to open file. Error '%s'", *ErrorString);
        return nullptr;
    }
    else
    {
        return new FWindowsFileHandle(NewHandle);
    }
}

const CHAR* FWindowsPlatformFile::GetExecutablePath()
{
    static CHAR StaticExecutablePath[512] = { 0 };

    if(!StaticExecutablePath[0])
    {
        if (!GetModuleFileName(0, StaticExecutablePath, ARRAY_COUNT(StaticExecutablePath)))
        {
            StaticExecutablePath[0] = 0;
        }
    }

    return StaticExecutablePath;
}

FString FWindowsPlatformFile::GetCurrentWorkingDirectory()
{
    int32 Length = ::GetCurrentDirectoryA(0, nullptr);
    if (!Length)
    {
        FString Error;
        const int32 ErrorCode = FWindowsPlatformMisc::GetLastErrorString(Error);
        LOG_ERROR("GetCurrentWorkingDirectory failed with error %d '%s' ", ErrorCode, *Error);
        return FString();
    }

    FString Result;
    Result.Resize(Length);
    Length = ::GetCurrentDirectoryA(Result.Size(), Result.Data());
    if (!Length)
    {
        FString Error;
        const int32 ErrorCode = FWindowsPlatformMisc::GetLastErrorString(Error);
        LOG_ERROR("GetCurrentWorkingDirectory failed with error %d '%s' ", ErrorCode, *Error);
        return FString();
    }
    else
    {
        return Result;
    }
}
