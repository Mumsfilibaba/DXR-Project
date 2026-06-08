#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Stream.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformFile
{
    virtual ~IPlatformFile() = default;

    /** @brief Move the file pointer relative to the beginning of the file */
    virtual bool SeekFromStart(int64 NewPosition) = 0;
    
    /** @brief Move the file pointer relative to the current pointer of the file */
    virtual bool SeekFromCurrent(int64 NewPosition) = 0;

    /** @brief Move the file pointer relative to the end of the file */
    virtual bool SeekFromEnd(int64 NewPosition) = 0;

    /** @return Returns the size of the file */
    virtual int64 Size() const = 0;

    /** @return Returns the current pointer of the file */
    virtual int64 Tell() const = 0;

    /** @brief Read from the file */
    virtual int32 Read(uint8* Dst, uint32 BytesToRead) = 0;

    /** @brief Write to the file */
    virtual int32 Write(const uint8* Src, uint32 BytesToWrite) = 0;

    /** @brief Truncate the file if the file is currently larger than the new size */
    virtual bool Truncate(int64 NewSize) = 0;

    /** @return Returns true if the FileHandle is valid */
    virtual bool IsValid() const = 0;

    /** @brief Closes the FileHandle and deletes this instance */
    virtual void Close() = 0;
};

struct IPlatformAsyncFile
{
    virtual ~IPlatformAsyncFile() = default;

    /** @brief Submits an asynchronous write. Copies Src internally and returns immediately. */
    virtual bool WriteAsync(const uint8* Src, uint32 BytesToWrite) = 0;

    /** @brief Blocks until all pending async writes have completed */
    virtual void WaitForPendingWrites() = 0;

    /** @return Returns true if there are async writes still in-flight */
    virtual bool HasPendingWrites() const = 0;

    /** @return Returns true if the handle is valid */
    virtual bool IsValid() const = 0;

    /** @brief Waits for pending writes, closes the handle, and deletes this instance */
    virtual void Close() = 0;
};

template<typename T>
class TFileRef
{ 
public:
    FORCEINLINE TFileRef()
        : Handle(nullptr)
    {
    }

    FORCEINLINE TFileRef(T* InHandle)
        : Handle(InHandle)
    {
    }
    
    FORCEINLINE TFileRef(TFileRef&& Other)
        : Handle(Other.Handle)
    {
        Other.Handle = nullptr;
    }

    FORCEINLINE ~TFileRef()
    {
        Close();
    }

    FORCEINLINE bool IsValid() const
    {
        return (Handle != nullptr);
    }

    FORCEINLINE void Close()
    {
        if (Handle)
        {
            Handle->Close();
            Handle = nullptr;
        }
    }

    FORCEINLINE T* Get() const
    {
        return Handle;
    }

    FORCEINLINE T* operator->() const
    {
        return Handle;
    }

    FORCEINLINE operator bool()
    {
        return IsValid();
    }

    FORCEINLINE TFileRef& operator=(TFileRef&& Other)
    {
        if (this != ::AddressOf(Other))
        {
            Close();
            Handle = Other.Handle;
            Other.Handle = nullptr;
        }

        return *this;
    }

private:
    T* Handle;
};


struct CORE_API FGenericPlatformFile
{
    static void ObtainRelativePath(const FString& Path);

    static FORCEINLINE IPlatformFile* OpenForRead(const FString& Filename) 
    {
        return nullptr;
    }

    static FORCEINLINE IPlatformFile* OpenForWrite(const FString& Filename, bool bTruncate = true)
    {
        return nullptr;
    }

    static FORCEINLINE IPlatformAsyncFile* OpenForAsyncWrite(const FString& Filename, bool bTruncate = true)
    {
        return nullptr;
    }

    static FORCEINLINE FString GetCurrentWorkingDirectory()
    {
        return FString();
    }

    static FORCEINLINE const CHAR* GetExecutablePath()
    {
        return "";
    }

    static FORCEINLINE bool IsDirectory(const CHAR* Path)
    {
        return false;
    }

    static FORCEINLINE bool IsFile(const CHAR* Path)
    {
        return false;
    }

    static FORCEINLINE bool IsPathRelative(const CHAR* Path)
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
