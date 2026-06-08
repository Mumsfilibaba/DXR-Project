#include "Core/Mac/MacPlatformFile.h"
#include "Core/Platform/PlatformString.h"
#include "Core/Memory/Memory.h"
#include <aio.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

FMacFileHandle::FMacFileHandle(int32 InFileHandle, bool bInReadOnly)
    : IPlatformFile()
    , FileHandle(InFileHandle)
    , bReadOnly(bInReadOnly)
{
}

bool FMacFileHandle::SeekFromStart(int64 InOffset)
{
    CHECK(IsValid());
    return (::lseek(FileHandle, InOffset, SEEK_SET) != -1);
}

bool FMacFileHandle::SeekFromCurrent(int64 InOffset)
{
    CHECK(IsValid());
    return (::lseek(FileHandle, InOffset, SEEK_CUR) != -1);
}

bool FMacFileHandle::SeekFromEnd(int64 InOffset)
{
    CHECK(IsValid());
    return (::lseek(FileHandle, InOffset, SEEK_END) != -1);
}

int64 FMacFileHandle::Size() const
{
    struct stat FileInfo;
    ::fstat(FileHandle, &FileInfo);
    return FileInfo.st_size;
}

int64 FMacFileHandle::Tell() const
{
    CHECK(IsValid());
    return ::lseek(FileHandle, 0, SEEK_CUR);
}

int32 FMacFileHandle::Read(uint8* Dst, uint32 BytesToRead)
{
    CHECK(IsValid());
    CHECK(Dst != nullptr);

    int64 MaxReadSize = MaxReadWriteSize;   
    int64 BytesRead   = 0;
    while (BytesToRead)
    {
        const int64 Size = Math::Min<int64>(MaxReadSize, BytesToRead);
        const int64 Read = ::read(FileHandle, Dst, Size);
        if (Read >= 0)
        {
            // File was smaller so we are already finished
            if (Read != Size)
            {
                return static_cast<int32>(BytesRead);
            }

            // Update vars and read again to satisfy the BytesToRead
            BytesRead   += Read;
            Dst         += Size;
            BytesToRead -= Size;
            CHECK(BytesToRead >= 0);
        }
        else if (Read == -1)
        {
            if ((MaxReadSize > 1024) && (errno == EINVAL))
            {
                // We try to read again but with a smaller buffer
                MaxReadSize /= 2;
            }
            else
            {
                // The file descriptor was invalid
                return static_cast<int32>(BytesRead);
            }
        }
    }

    return static_cast<int32>(BytesRead);
}

int32 FMacFileHandle::Write(const uint8* Src, uint32 BytesToWrite)
{
    CHECK(IsValid());
    CHECK(Src != nullptr);

    int64 BytesWritten = 0;
    while (BytesToWrite)
    {
        const int64 Size    = Math::Min<int64>(MaxReadWriteSize, BytesToWrite);
        const int64 Written = ::write(FileHandle, Src, Size);
        BytesWritten += Written;

        if (Written != Size)
        {
            break;
        }

        Src          += Size;
        BytesToWrite -= Size;
        CHECK(BytesToWrite >= 0);
    }

    return static_cast<int32>(BytesWritten);
}

bool FMacFileHandle::Truncate(int64 NewSize)
{
    CHECK(IsValid());
    
    int32 Result = 0;
    do 
    { 
        Result = ::ftruncate(FileHandle, NewSize);
    } while ((Result < 0) && (errno == EINTR));
    
    return Result == 0;
}

bool FMacFileHandle::IsValid() const
{
    return (FileHandle >= 0);
}

void FMacFileHandle::Close()
{   
    if (IsValid())
    {
        if (!bReadOnly)
        {
            const int32 Result = ::fsync(FileHandle);
            CHECK(Result >= 0);
        }

        // Unlock the file
        ::flock(FileHandle, LOCK_UN | LOCK_NB);

        {
            const int32 Result = ::close(FileHandle);
            CHECK(Result >= 0);
        }
    }
    
    FileHandle = -1;
    delete this;
}

struct FMacAsyncFileHandle::FPendingWrite
{
    struct aiocb ControlBlock;
    uint8*       Buffer;
};

FMacAsyncFileHandle::FMacAsyncFileHandle(int32 InFileDescriptor)
    : FileDescriptor(InFileDescriptor)
    , WriteOffset(0)
{
}

bool FMacAsyncFileHandle::WriteAsync(const uint8* Src, uint32 BytesToWrite)
{
    CHECK(IsValid());
    CHECK(Src != nullptr);
    CHECK(BytesToWrite > 0);

    GarbageCollectCompleted();

    FPendingWrite* Pending = new FPendingWrite();
    Memory::Memzero(&Pending->ControlBlock, sizeof(struct aiocb));

    Pending->Buffer = reinterpret_cast<uint8*>(Memory::Malloc(BytesToWrite));
    Memory::Memcpy(Pending->Buffer, Src, BytesToWrite);

    Pending->ControlBlock.aio_fildes = FileDescriptor;
    Pending->ControlBlock.aio_buf    = Pending->Buffer;
    Pending->ControlBlock.aio_nbytes = BytesToWrite;
    Pending->ControlBlock.aio_offset = WriteOffset;

    int32 Result = ::aio_write(&Pending->ControlBlock);
    if (Result != 0)
    {
        Memory::Free(Pending->Buffer);
        delete Pending;
        return false;
    }

    WriteOffset += BytesToWrite;
    PendingWrites.Emplace(Pending);
    return true;
}

void FMacAsyncFileHandle::WaitForPendingWrites()
{
    while (!PendingWrites.IsEmpty())
    {
        TArray<struct aiocb*> AioCBs;
        AioCBs.Resize(PendingWrites.Size());

        for (int32 i = 0; i < PendingWrites.Size(); ++i)
        {
            AioCBs[i] = &PendingWrites[i]->ControlBlock;
        }

        ::aio_suspend(AioCBs.Data(), AioCBs.Size(), nullptr);

        for (int32 i = PendingWrites.Size() - 1; i >= 0; --i)
        {
            int32 Error = ::aio_error(&PendingWrites[i]->ControlBlock);
            if (Error != EINPROGRESS)
            {
                ::aio_return(&PendingWrites[i]->ControlBlock);
                FreePendingWrite(PendingWrites[i]);
                PendingWrites.RemoveAt(i);
            }
        }
    }
}

bool FMacAsyncFileHandle::HasPendingWrites() const
{
    const_cast<FMacAsyncFileHandle*>(this)->GarbageCollectCompleted();
    return !PendingWrites.IsEmpty();
}

bool FMacAsyncFileHandle::IsValid() const
{
    return (FileDescriptor >= 0);
}

void FMacAsyncFileHandle::Close()
{
    WaitForPendingWrites();

    if (IsValid())
    {
        ::flock(FileDescriptor, LOCK_UN | LOCK_NB);
        ::close(FileDescriptor);
    }

    FileDescriptor = -1;
    delete this;
}

void FMacAsyncFileHandle::GarbageCollectCompleted()
{
    for (int32 i = PendingWrites.Size() - 1; i >= 0; --i)
    {
        int32 Error = ::aio_error(&PendingWrites[i]->ControlBlock);
        if (Error != EINPROGRESS)
        {
            ::aio_return(&PendingWrites[i]->ControlBlock);
            FreePendingWrite(PendingWrites[i]);
            PendingWrites.RemoveAt(i);
        }
    }
}

void FMacAsyncFileHandle::FreePendingWrite(FPendingWrite* PendingWrite)
{
    Memory::Free(PendingWrite->Buffer);
    delete PendingWrite;
}

IPlatformAsyncFile* FMacPlatformFile::OpenForAsyncWrite(const FString& Filename, bool bTruncate)
{
    int32 Flags =
        O_WRONLY |
        O_CREAT;

    if (bTruncate)
    {
        Flags |= O_TRUNC;
    }

    const int32 PermissionFlags =
        S_IRUSR | S_IWUSR |
        S_IRGRP | S_IWGRP |
        S_IROTH | S_IWOTH;

    int32 FileHandle = ::open(*Filename, Flags, PermissionFlags);
    if (FileHandle < 0)
    {
        return nullptr;
    }

    const int32 LockFlags = LOCK_NB | LOCK_EX;
    const int32 Result = ::flock(FileHandle, LockFlags);
    if (Result != 0)
    {
        ::close(FileHandle);
        return nullptr;
    }

    return new FMacAsyncFileHandle(FileHandle);
}

IPlatformFile* FMacPlatformFile::OpenForRead(const FString& Filename)
{
    int32 FileHandle = ::open(*Filename, O_RDONLY);
    if (FileHandle < 0)
    {
        return nullptr;
    }

    const int32 LockFlags = 
        LOCK_NB | // Do not block, return error instead
        LOCK_SH ; // Shared lock, since we are reading

    const int32 Result = ::flock(FileHandle, LockFlags);
    if (Result != 0)
    {
        ::close(FileHandle);
        return nullptr;
    }
    else
    {
        return new FMacFileHandle(FileHandle, true);
    }
}

IPlatformFile* FMacPlatformFile::OpenForWrite(const FString& Filename, bool bTruncate)
{
    int32 Flags =
        O_WRONLY | // Writing only 
        O_CREAT;   // Create if the file does not exist
    
    if (bTruncate)
    {
        Flags |= O_TRUNC; // Truncate the file if it exists
    }

    const int32 PermissonFlags = 
        S_IRUSR | // Read permission for User
        S_IWUSR | // Write permission for User
        S_IRGRP | // Read permission for Group
        S_IWGRP | // Write permission for Group
        S_IROTH | // Read permission for Other
        S_IWOTH;  // Write permission for Other

    int32 FileHandle = ::open(*Filename, Flags, PermissonFlags);
    if (FileHandle < 0)
    {
        return nullptr;
    }

    const int32 LockFlags = 
        LOCK_NB | // Do not block, return error instead
        LOCK_EX ; // Exclusive lock, since we are writing

    const int32 Result = ::flock(FileHandle, LockFlags);
    if (Result != 0)
    {
        ::close(FileHandle);
        return nullptr;
    }
    else
    {
        return new FMacFileHandle(FileHandle, false);
    }
}

FString FMacPlatformFile::GetCurrentWorkingDirectory()
{
    CHAR Buffer[MAXPATHLEN] = { 0 };
    CHAR* CurrentDirectory = ::getcwd(Buffer, sizeof(Buffer));
    if (!CurrentDirectory)
    {
        return FString();
    }
    else
    {
        return CurrentDirectory;
    }
}

const CHAR* FMacPlatformFile::GetExecutablePath()
{
    static CHAR StaticExecutablePath[MAXPATHLEN] = { 0 };

    if (!StaticExecutablePath[0])
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* ExecutablePathNS = [[NSBundle mainBundle] executablePath];
        const CHAR* ExecutablePath = [ExecutablePathNS UTF8String];
        
        const uint64 Length = FPlatformString::Strlen(ExecutablePath);
        FPlatformString::Strncpy(StaticExecutablePath, ExecutablePath, MAXPATHLEN);
        StaticExecutablePath[Length] = 0;
    }

    return StaticExecutablePath;
}
