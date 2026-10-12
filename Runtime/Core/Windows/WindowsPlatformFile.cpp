#include "Core/Windows/WindowsPlatformFile.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Windows/WindowsPlatformMisc.h"
#include "Core/Templates/CString.h"
#include "Core/Templates/NumericLimits.h"

#include <objbase.h>
#include <shlobj.h>

// FILETIME counts 100-nanosecond intervals since 1601-01-01, which is this many intervals before the Unix epoch
static constexpr int64 FILETIME_TICKS_PER_SECOND = 10000000;
static constexpr int64 FILETIME_UNIX_EPOCH       = 116444736000000000;

static int64 FileTimeToUnixTime(const FILETIME& InFileTime)
{
    ULARGE_INTEGER Ticks;
    Ticks.LowPart  = InFileTime.dwLowDateTime;
    Ticks.HighPart = InFileTime.dwHighDateTime;
    return (static_cast<int64>(Ticks.QuadPart) - FILETIME_UNIX_EPOCH) / FILETIME_TICKS_PER_SECOND;
}

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
    return Seek(InOffset, FILE_BEGIN);
}

bool FWindowsFileHandle::SeekFromCurrent(int64 InOffset)
{
    return Seek(InOffset, FILE_CURRENT);
}

bool FWindowsFileHandle::SeekFromEnd(int64 InOffset)
{
    return Seek(InOffset, FILE_END);
}

bool FWindowsFileHandle::Seek(int64 InOffset, DWORD MoveMethod)
{
    CHECK(IsValid());

    LARGE_INTEGER Offset;
    Offset.QuadPart = InOffset;

    LARGE_INTEGER NewPosition;
    if (!::SetFilePointerEx(FileHandle, Offset, &NewPosition, MoveMethod))
    {
        return false;
    }

    FilePointer = static_cast<int64>(NewPosition.QuadPart);
    return true;
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
        const DWORD Error = GetLastError();

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


IPlatformFile* FWindowsPlatformFile::OpenForRead(const FString& Filename)
{
    ::SetLastError(S_OK);

    // Readers must not lock each other out; permutations of one shader are compiled in parallel and share a source file.
    HANDLE NewHandle = ::CreateFileW(StringCast<WIDECHAR>(Filename).Get(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD LastError = ::GetLastError();

        FString ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);
        
        int32 Position = ErrorString.FindLast(TEXT("\r\n"));
        if (Position != FString::InvalidIndex)
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

IPlatformFile* FWindowsPlatformFile::OpenForWrite(const FString& Filename, bool bTruncate)
{
    ::SetLastError(S_OK);

    const DWORD CreationDisposition = bTruncate ? CREATE_ALWAYS : OPEN_ALWAYS;

    HANDLE NewHandle = ::CreateFileW(StringCast<WIDECHAR>(Filename).Get(), GENERIC_WRITE, 0, 0, CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD LastError = ::GetLastError();

        FString ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);

        int32 Position = ErrorString.FindLast(TEXT("\r\n"));
        if (Position != FString::InvalidIndex)
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

IPlatformAsyncFile* FWindowsPlatformFile::OpenForAsyncWrite(const FString& Filename, bool bTruncate)
{
    ::SetLastError(S_OK);

    const DWORD CreationDisposition = bTruncate ? CREATE_ALWAYS : OPEN_ALWAYS;

    HANDLE NewHandle = ::CreateFileW(StringCast<WIDECHAR>(Filename).Get(), GENERIC_WRITE, 0, 0, CreationDisposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if (NewHandle == INVALID_HANDLE_VALUE)
    {
        FString ErrorString;
        FWindowsPlatformMisc::GetLastErrorString(ErrorString);

        int32 Position = ErrorString.FindLast(TEXT("\r\n"));
        if (Position != FString::InvalidIndex)
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

    SCOPED_LOCK(PendingWritesCS);

    GarbageCollectCompleted();

    FPendingWrite* Pending = new FPendingWrite();
    Memory::Memzero(&Pending->Overlapped, sizeof(OVERLAPPED));

    Pending->Buffer = reinterpret_cast<uint8*>(Memory::Malloc(BytesToWrite));
    Memory::Memcpy(Pending->Buffer, Src, BytesToWrite);

    Pending->CompletionEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (Pending->CompletionEvent == nullptr)
    {
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
                ReportWriteFailure(TEXT("Blocking write failed"), Error);
                return false;
            }
        }

        DWORD Written = 0;
        if (!::GetOverlappedResult(FileHandle, &Overlapped, &Written, TRUE))
        {
            ReportWriteFailure(TEXT("Blocking write failed to complete"), ::GetLastError());
            return false;
        }

        if (Written == 0)
        {
            ReportWriteFailure(TEXT("Blocking write accepted no bytes"), ERROR_WRITE_FAULT);
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
        ReportWriteFailure(TEXT("Async write failed"), ::GetLastError());
    }
}

void FWindowsAsyncFileHandle::ReportWriteFailure(const CHAR_T* What, uint32 ErrorCode)
{
    if (bHasWriteError)
    {
        return;
    }

    bHasWriteError = true;

    ::SetLastError(static_cast<DWORD>(ErrorCode));

    FString ErrorString;
    FWindowsPlatformMisc::GetLastErrorString(ErrorString);

    const int32 Position = ErrorString.FindLast(TEXT("\r\n"));
    if (Position != FString::InvalidIndex)
    {
        ErrorString.Remove(Position, 2);
    }

    LOG_ERROR("[FWindowsAsyncFileHandle] %s. Error '%s'", What, *ErrorString);
}

void FWindowsAsyncFileHandle::WaitForPendingWrites()
{
    TArray<FPendingWrite*> WritesToWaitFor;
    {
        SCOPED_LOCK(PendingWritesCS);
        WritesToWaitFor = Move(PendingWrites);
        PendingWrites.Clear();
    }

    for (FPendingWrite* Pending : WritesToWaitFor)
    {
        ::WaitForSingleObject(Pending->CompletionEvent, INFINITE);
        CollectWriteResult(Pending, true);
        FreePendingWrite(Pending);
    }
}

bool FWindowsAsyncFileHandle::HasPendingWrites() const
{
    SCOPED_LOCK(PendingWritesCS);

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

const CHAR_T* FWindowsPlatformFile::GetExecutablePath()
{
    static FString StaticExecutablePath;

    if (StaticExecutablePath.IsEmpty())
    {
        TArray<WIDECHAR> WidePath;
        WidePath.Resize(MAX_PATH);

        DWORD Length = 0;
        while (true)
        {
            Length = ::GetModuleFileNameW(nullptr, WidePath.Data(), static_cast<DWORD>(WidePath.Size()));
            if (Length < static_cast<DWORD>(WidePath.Size()) || WidePath.Size() >= 32768)
            {
                break;
            }

            WidePath.Resize(WidePath.Size() * 2);
        }

        StaticExecutablePath = StringCast<CHAR_T>(WidePath.Data(), static_cast<int32>(Length)).ToString();
        StaticExecutablePath.ReplaceAll('\\', '/');
    }

    return *StaticExecutablePath;
}

FString FWindowsPlatformFile::GetCurrentWorkingDirectory()
{
    const int32 RequiredSize = ::GetCurrentDirectoryW(0, nullptr);
    if (!RequiredSize)
    {
        FString Error;
        const int32 ErrorCode = FWindowsPlatformMisc::GetLastErrorString(Error);
        LOG_ERROR("GetCurrentWorkingDirectory failed with error %d '%s' ", ErrorCode, *Error);
        return FString();
    }

    FWideString WideResult;
    WideResult.Resize(RequiredSize - 1);

    const int32 Length = ::GetCurrentDirectoryW(RequiredSize, WideResult.Data());
    if (!Length || Length >= RequiredSize)
    {
        FString Error;

        const int32 ErrorCode = FWindowsPlatformMisc::GetLastErrorString(Error);
        LOG_ERROR("GetCurrentWorkingDirectory failed with error %d '%s' ", ErrorCode, *Error);
        return FString();
    }
    else
    {
        FString Result = StringCast<CHAR_T>(WideResult.Data(), Length).ToString();
        Result.ReplaceAll('\\', '/');
        return Result;
    }
}

FString FWindowsPlatformFile::GetUserSettingsDirectory()
{
    PWSTR WidePath = nullptr;
    if (FAILED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &WidePath)))
    {
        ::CoTaskMemFree(WidePath);
        LOG_ERROR("GetUserSettingsDirectory failed to find the local application data directory");
        return FString();
    }

    FString Path = StringCast<CHAR_T>(WidePath).ToString();
    ::CoTaskMemFree(WidePath);

    Path.ReplaceAll('\\', '/');
    return Path;
}

FFileInfo FWindowsPlatformFile::GetFileInfo(const CHAR_T* Path)
{
    FFileInfo Info;

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (!::GetFileAttributesExW(StringCast<WIDECHAR>(Path).Get(), GetFileExInfoStandard, &Data))
    {
        return Info;
    }

    Info.bExists      = true;
    Info.bIsDirectory = (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    Info.ModifiedTime = FileTimeToUnixTime(Data.ftLastWriteTime);

    if (!Info.bIsDirectory)
    {
        ULARGE_INTEGER FileSize;
        FileSize.LowPart  = Data.nFileSizeLow;
        FileSize.HighPart = Data.nFileSizeHigh;
        Info.Size         = static_cast<int64>(FileSize.QuadPart);
    }

    return Info;
}

bool FWindowsPlatformFile::IterateDirectory(const CHAR_T* Path, TArray<FDirectoryEntry>& OutEntries)
{
    OutEntries.Clear();

    WIN32_FIND_DATAW FindData;
    const FString    Search = FString::Printf(TEXT("%s\\*"), Path);
    const HANDLE     Handle = ::FindFirstFileW(StringCast<WIDECHAR>(Search).Get(), &FindData);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    do
    {
        if ((CStringWide::Strcmp(FindData.cFileName, L".") == 0) || (CStringWide::Strcmp(FindData.cFileName, L"..") == 0))
        {
            continue;
        }

        const bool bIsReparsePoint = (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        const bool bIsLinkTag      = (FindData.dwReserved0 == IO_REPARSE_TAG_SYMLINK) || (FindData.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT);

        FDirectoryEntry Entry;
        Entry.Name            = StringCast<CHAR_T>(FindData.cFileName).ToString();
        Entry.bIsDirectory    = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        Entry.bIsSymbolicLink = bIsReparsePoint && bIsLinkTag;
        OutEntries.Add(Move(Entry));
    }
    while (::FindNextFileW(Handle, &FindData));

    ::FindClose(Handle);
    return true;
}
