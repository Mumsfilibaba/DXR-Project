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

FWindowsFileHandle::~FWindowsFileHandle() = default;

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
        
            // ERROR_IO_PENDING only means the operation has not landed yet
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

        // ERROR_IO_PENDING only means the operation has not landed yet
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

    // Readers must not lock each other out; permutations of one shader are compiled in parallel and share a source file.
    HANDLE NewHandle = ::CreateFileA(*Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD LastError = ::GetLastError();

        String ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);
        
        int32 Position = ErrorString.FindLast("\r\n");
        if (Position != String::InvalidIndex)
        {
            ErrorString.Remove(Position, 2);
        }

        if (LastError == ERROR_FILE_NOT_FOUND || LastError == ERROR_PATH_NOT_FOUND)
        {
            // A missing file is the caller's decision to handle, not inherently an error
            LOG_WARNING("[FWindowsPlatformFile] File not found '%s'", *Filename);
        }
        else
        {
            LOG_ERROR("[FWindowsPlatformFile] Failed to open file. Error '%s'", *ErrorString);
        }
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
        const DWORD LastError = ::GetLastError();

        String ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);

        int32 Position = ErrorString.FindLast("\r\n");
        if (Position != String::InvalidIndex)
        {
            ErrorString.Remove(Position, 2);
        }

        if (LastError == ERROR_FILE_NOT_FOUND || LastError == ERROR_PATH_NOT_FOUND)
        {
            // A missing path is the caller's decision to handle, not inherently an error
            LOG_WARNING("[FWindowsPlatformFile] File not found '%s'", *Filename);
        }
        else
        {
            LOG_ERROR("[FWindowsPlatformFile] Failed to open file. Error '%s'", *ErrorString);
        }
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
    , bHasWriteError(false)
{
}

FWindowsAsyncFileHandle::~FWindowsAsyncFileHandle() = default;

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
        // With no event there is nothing to track the write with, so it goes down synchronously.
        const bool bWritten = WriteBlockingAtOffset(Pending->Buffer, BytesToWrite, WriteOffset);

        Memory::Free(Pending->Buffer);
        delete Pending;

        if (!bWritten)
        {
            return false;
        }

        WriteOffset += BytesToWrite;
        return true;
    }

    Pending->Overlapped.Offset     = static_cast<DWORD>(WriteOffset & 0xFFFFFFFF);
    Pending->Overlapped.OffsetHigh = static_cast<DWORD>((WriteOffset >> 32) & 0xFFFFFFFF);
    Pending->Overlapped.hEvent     = Pending->CompletionEvent;

    BOOL  Result = ::WriteFile(FileHandle, Pending->Buffer, BytesToWrite, nullptr, &Pending->Overlapped);
    DWORD Error  = Result ? ERROR_SUCCESS : ::GetLastError();

    if (!Result && Error == ERROR_NOT_ENOUGH_QUOTA)
    {
        // An overlapped write locks its buffer into memory against a per-process quota, so a burst
        // exhausts it. Collecting the writes that have already landed releases their pages again.
        WaitForPendingWrites();

        Memory::Memzero(&Pending->Overlapped, sizeof(OVERLAPPED));
        Pending->Overlapped.Offset     = static_cast<DWORD>(WriteOffset & 0xFFFFFFFF);
        Pending->Overlapped.OffsetHigh = static_cast<DWORD>((WriteOffset >> 32) & 0xFFFFFFFF);
        Pending->Overlapped.hEvent     = Pending->CompletionEvent;

        Result = ::WriteFile(FileHandle, Pending->Buffer, BytesToWrite, nullptr, &Pending->Overlapped);
        Error  = Result ? ERROR_SUCCESS : ::GetLastError();
    }

    if (!Result && Error != ERROR_IO_PENDING)
    {
        // A caller cannot tell a refused write from one that has merely not landed, so stall rather than lose the bytes.
        const bool bWritten = WriteBlockingAtOffset(Pending->Buffer, BytesToWrite, WriteOffset);
        FreePendingWrite(Pending);

        if (!bWritten)
        {
            return false;
        }

        WriteOffset += BytesToWrite;
        return true;
    }

    WriteOffset += BytesToWrite;
    PendingWrites.Emplace(Pending);
    return true;
}

bool FWindowsAsyncFileHandle::WriteBlockingAtOffset(const uint8* Src, uint32 BytesToWrite, int64 Offset)
{
    // This write carries no event of its own, so GetOverlappedResult waits on the file handle, which
    // any outstanding write would also signal. Draining first leaves nothing else to wake it.
    WaitForPendingWrites();

    uint32 TotalWritten = 0;
    while (TotalWritten < BytesToWrite)
    {
        const int64 CurrentOffset = Offset + TotalWritten;

        OVERLAPPED Overlapped;
        Memory::Memzero(&Overlapped, sizeof(OVERLAPPED));
        Overlapped.Offset     = static_cast<DWORD>(CurrentOffset & 0xFFFFFFFF);
        Overlapped.OffsetHigh = static_cast<DWORD>((CurrentOffset >> 32) & 0xFFFFFFFF);

        if (!::WriteFile(FileHandle, Src + TotalWritten, BytesToWrite - TotalWritten, nullptr, &Overlapped))
        {
            const DWORD Error = ::GetLastError();
            if (Error != ERROR_IO_PENDING)
            {
                ReportWriteFailure("Blocking write failed", Error);
                return false;
            }
        }

        // The handle is overlapped, so the result is collected here even when the write completed at once
        DWORD Written = 0;
        if (!::GetOverlappedResult(FileHandle, &Overlapped, &Written, TRUE))
        {
            ReportWriteFailure("Blocking write failed to complete", ::GetLastError());
            return false;
        }

        if (Written == 0)
        {
            // A zero return for a non-empty request would leave the loop turning without progress.
            ReportWriteFailure("Blocking write accepted no bytes", ERROR_WRITE_FAULT);
            return false;
        }

        TotalWritten += Written;
    }

    return true;
}

void FWindowsAsyncFileHandle::CollectWriteResult(FPendingWrite* PendingWrite, bool bWait)
{
    DWORD Written = 0;
    if (!::GetOverlappedResult(FileHandle, &PendingWrite->Overlapped, &Written, bWait ? TRUE : FALSE))
    {
        ReportWriteFailure("Async write failed", ::GetLastError());
    }
}

void FWindowsAsyncFileHandle::ReportWriteFailure(const CHAR* What, uint32 ErrorCode)
{
    if (bHasWriteError)
    {
        return;
    }

    // Reporting goes through the log, so the line comes back to this handle. Raising the flag first
    // means the flush that eventually carries it finds a handle that has already failed and drops the
    // batch, rather than driving another write into the same failure.
    bHasWriteError = true;

    ::SetLastError(static_cast<DWORD>(ErrorCode));

    String ErrorString;
    FWindowsPlatformMisc::GetLastErrorString(ErrorString);

    const int32 Position = ErrorString.FindLast("\r\n");
    if (Position != String::InvalidIndex)
    {
        ErrorString.Remove(Position, 2);
    }

    LOG_ERROR("[FWindowsAsyncFileHandle] %s. Error '%s'", What, *ErrorString);
}

void FWindowsAsyncFileHandle::WaitForPendingWrites()
{
    for (FPendingWrite* Pending : PendingWrites)
    {
        ::WaitForSingleObject(Pending->CompletionEvent, INFINITE);
        CollectWriteResult(Pending, true);
        FreePendingWrite(Pending);
    }

    PendingWrites.Clear();
}

bool FWindowsAsyncFileHandle::HasPendingWrites() const
{
    const_cast<FWindowsAsyncFileHandle*>(this)->GarbageCollectCompleted();
    return !PendingWrites.IsEmpty();
}

bool FWindowsAsyncFileHandle::HasWriteError() const
{
    return bHasWriteError;
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
            CollectWriteResult(PendingWrites[i], false);
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
