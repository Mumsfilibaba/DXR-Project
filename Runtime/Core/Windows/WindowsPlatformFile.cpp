#include "Core/Windows/WindowsPlatformFile.h"
#include "Core/Windows/WindowsPlatformMisc.h"
#include "Core/Templates/NumericLimits.h"

FWindowsFileHandle::FWindowsFileHandle(HANDLE InFileHandle)
    : IPlatformFile()
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
        const uint32 LocalBytesToRead = Math::Min<uint32>(BytesToRead, TNumericLimits<uint32>::Max());

        DWORD NumRead = 0;
        if (!ReadFile(FileHandle, Dst, LocalBytesToRead, &NumRead, nullptr))
        {
            const int32 Error = ::GetLastError();
        
            // ERROR_IO_PENDING is not an error, however if the error is not that we report an error
            if (Error != ERROR_IO_PENDING)
            {
                String ErrorString;

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
    return FileHandle != 0 && FileHandle != INVALID_HANDLE_VALUE && FileSize != -1;
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

void FWindowsFileHandle::UpdateFileSize()
{
    if (FileHandle != 0 && FileHandle != INVALID_HANDLE_VALUE)
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


IPlatformFile* FWindowsPlatformFile::OpenForRead(const String& Filename)
{
    ::SetLastError(S_OK);

    HANDLE NewHandle = CreateFileA(*Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        String ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);
        
        int32 Position = ErrorString.FindLast("\r\n");
        if (Position != String::InvalidIndex)
        {
            ErrorString.Remove(Position, 2);
        }

        LOG_ERROR("[FWindowsPlatformFile] Failed to open file. Error '%s'", *ErrorString);
        return nullptr;
    }
    else
    {
        ::SetLastError(S_OK);
        return new FWindowsFileHandle(NewHandle);
    }
}

IPlatformFile* FWindowsPlatformFile::OpenForWrite(const String& Filename, bool bTruncate)
{
    ::SetLastError(S_OK);

    const DWORD CreationDisposition = bTruncate ? CREATE_ALWAYS : OPEN_ALWAYS;

    HANDLE NewHandle = ::CreateFileA(*Filename, GENERIC_WRITE, 0, 0, CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        String ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);

        int32 Position = ErrorString.FindLast("\r\n");
        if (Position != String::InvalidIndex)
        {
            ErrorString.Remove(Position, 2);
        }

        LOG_ERROR("[FWindowsPlatformFile] Failed to open file. Error '%s'", *ErrorString);
        return nullptr;
    }
    else
    {
        ::SetLastError(S_OK);
        return new FWindowsFileHandle(NewHandle);
    }
}

IPlatformAsyncFile* FWindowsPlatformFile::OpenForAsyncWrite(const String& Filename, bool bTruncate)
{
    ::SetLastError(S_OK);

    const DWORD CreationDisposition = bTruncate ? CREATE_ALWAYS : OPEN_ALWAYS;

    HANDLE NewHandle = ::CreateFileA(*Filename, GENERIC_WRITE, 0, 0, CreationDisposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        String ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);

        int32 Position = ErrorString.FindLast("\r\n");
        if (Position != String::InvalidIndex)
        {
            ErrorString.Remove(Position, 2);
        }

        LOG_ERROR("[FWindowsPlatformFile] Failed to open async file. Error '%s'", *ErrorString);
        return nullptr;
    }
    else
    {
        ::SetLastError(S_OK);
        return new FWindowsAsyncFileHandle(NewHandle);
    }
}

FWindowsAsyncFileHandle::FWindowsAsyncFileHandle(HANDLE InFileHandle)
    : FileHandle(InFileHandle)
    , WriteOffset(0)
{
}

bool FWindowsAsyncFileHandle::WriteAsync(const uint8* Src, uint32 BytesToWrite)
{
    CHECK(IsValid());
    CHECK(Src != nullptr);
    CHECK(BytesToWrite > 0);

    GarbageCollectCompleted();

    FPendingWrite* Pending = new FPendingWrite();
    Memory::Memzero(&Pending->Overlapped, sizeof(OVERLAPPED));

    Pending->Buffer = reinterpret_cast<uint8*>(Memory::Malloc(BytesToWrite));
    Memory::Memcpy(Pending->Buffer, Src, BytesToWrite);

    Pending->CompletionEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (Pending->CompletionEvent == nullptr)
    {
        Memory::Free(Pending->Buffer);
        delete Pending;
        return false;
    }

    Pending->Overlapped.Offset     = static_cast<DWORD>(WriteOffset & 0xFFFFFFFF);
    Pending->Overlapped.OffsetHigh = static_cast<DWORD>((WriteOffset >> 32) & 0xFFFFFFFF);
    Pending->Overlapped.hEvent     = Pending->CompletionEvent;

    BOOL Result = ::WriteFile(FileHandle, Pending->Buffer, BytesToWrite, nullptr, &Pending->Overlapped);
    if (!Result)
    {
        DWORD Error = ::GetLastError();
        if (Error != ERROR_IO_PENDING)
        {
            ::CloseHandle(Pending->CompletionEvent);
            Memory::Free(Pending->Buffer);
            delete Pending;
            return false;
        }
    }

    WriteOffset += BytesToWrite;
    PendingWrites.Emplace(Pending);
    return true;
}

void FWindowsAsyncFileHandle::WaitForPendingWrites()
{
    for (FPendingWrite* Pending : PendingWrites)
    {
        ::WaitForSingleObject(Pending->CompletionEvent, INFINITE);
        FreePendingWrite(Pending);
    }

    PendingWrites.Clear();
}

bool FWindowsAsyncFileHandle::HasPendingWrites() const
{
    const_cast<FWindowsAsyncFileHandle*>(this)->GarbageCollectCompleted();
    return !PendingWrites.IsEmpty();
}

bool FWindowsAsyncFileHandle::IsValid() const
{
    return FileHandle != nullptr && FileHandle != INVALID_HANDLE_VALUE;
}

void FWindowsAsyncFileHandle::Close()
{
    WaitForPendingWrites();

    if (IsValid())
    {
        ::CloseHandle(FileHandle);
    }

    FileHandle = INVALID_HANDLE_VALUE;
    delete this;
}

void FWindowsAsyncFileHandle::GarbageCollectCompleted()
{
    for (int32 i = PendingWrites.Size() - 1; i >= 0; --i)
    {
        if (HasOverlappedIoCompleted(&PendingWrites[i]->Overlapped))
        {
            FreePendingWrite(PendingWrites[i]);
            PendingWrites.RemoveAt(i);
        }
    }
}

void FWindowsAsyncFileHandle::FreePendingWrite(FPendingWrite* PendingWrite)
{
    ::CloseHandle(PendingWrite->CompletionEvent);
    Memory::Free(PendingWrite->Buffer);
    delete PendingWrite;
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

String FWindowsPlatformFile::GetCurrentWorkingDirectory()
{
    int32 Length = ::GetCurrentDirectoryA(0, nullptr);
    if (!Length)
    {
        String Error;
        const int32 ErrorCode = FWindowsPlatformMisc::GetLastErrorString(Error);
        LOG_ERROR("GetCurrentWorkingDirectory failed with error %d '%s' ", ErrorCode, *Error);
        return String();
    }

    String Result;
    Result.Resize(Length);

    Length = ::GetCurrentDirectoryA(Result.Size(), Result.Data());
    if (!Length)
    {
        String Error;

        const int32 ErrorCode = FWindowsPlatformMisc::GetLastErrorString(Error);
        LOG_ERROR("GetCurrentWorkingDirectory failed with error %d '%s' ", ErrorCode, *Error);
        return String();
    }
    else
    {
        return Result;
    }
}
