#include "MacFile.h"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacFileHandle

FMacFileHandle::FMacFileHandle(int32 InFileHandle, bool bInReadOnly)
    : IFileHandle()
    , FileHandle(InFileHandle)
    , bReadOnly(bInReadOnly)
{ }

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

    int64 MaxReadSize = kMaxReadWriteSize;   
    int64 BytesRead   = 0;
    while (BytesToRead)
    {
        const int64 Size = NMath::Min<int64>(MaxReadSize, BytesToRead);
        const int64 Read = ::read(FileHandle, Dst, Size);
        if (Read >= 0)
        {
            // File was smaller so we are already finished
            if (Read != Size)
            {
                return BytesRead;
            }

            // Update vars and read again to satisfy the BytesToRead
            BytesRead   += Read;         
            Dst         += Size;       
            BytesToRead -= Size;
            CHECK(BytesToRead >= 0);
        }
        else if (Read == -1)
        {
            if ((MaxReadSize > 1024LLU) && (errno == EINVAL))
            {
                // We try to read again but with a smaller buffer
                MaxReadSize /= 2;
            }
            else
            {
                // The file descriptor was invalid
                return BytesRead;
            }
        }
    }

    return BytesRead;
}

int32 FMacFileHandle::Write(const uint8* Src, uint32 BytesToWrite)
{
	CHECK(IsValid());
    CHECK(Src != nullptr);

    int64 BytesWritten = 0;
    while (BytesToWrite)
    {
        const int64 Size    = NMath::Min<int64>(kMaxReadWriteSize, BytesToWrite);
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

    return BytesWritten;
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
            const auto Result = ::fsync(FileHandle);
            CHECK(Result >= 0);
        }

        // Unlock the file
        ::flock(FileHandle, LOCK_UN | LOCK_NB);

        {
		    const auto Result = ::close(FileHandle);
            CHECK(Result >= 0);
        }
    }
    
    FileHandle = -1;
    delete this;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacFile

IFileHandle* FMacFile::OpenForRead(const FString& Filename)
{
    int32 FileHandle = ::open(Filename.GetCString(), O_RDONLY);
    if (FileHandle < 0)
    {
        return nullptr;
    }

    const int32 LockFlags = 
        LOCK_NB | // Do not block, return error instead
        LOCK_SH ; // Shared lock, since we are reading

    const auto Result = flock(FileHandle, LockFlags);
    if (Result != 0)
    {
        ::close(FileHandle);
        return nullptr;
    }

    return dbg_new FMacFileHandle(FileHandle, true);
}

IFileHandle* FMacFile::OpenForWrite(const FString& Filename)
{
    const int32 Flags = 
        O_WRONLY | // Writing only 
        O_CREAT  ;   // Create if the file does not exist

    const int32 PermissonFlags = 
        S_IRUSR | // Read for Permission for User
        S_IWUSR | // Write for Permission for User
        S_IRGRP | // Read for Permission for Group
        S_IWGRP | // Write for Permission for Group
        S_IROTH | // Read for Permission for Other
        S_IWOTH ; // Write for Permission for Other

    int32 FileHandle = ::open(Filename.GetCString(), Flags, PermissonFlags);
    if (FileHandle < 0)
    {
        return nullptr;
    }

    const int32 LockFlags = 
        LOCK_NB | // Do not block, return error instead
        LOCK_EX ; // Exclusive lock, since we are writing

    const auto Result = ::flock(FileHandle, LockFlags);
    if (Result != 0)
    {
        ::close(FileHandle);
        return nullptr;
    }

    return dbg_new FMacFileHandle(FileHandle, false);
}
