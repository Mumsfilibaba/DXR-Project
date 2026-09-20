#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
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

struct FDirectoryEntry
{
    String Name;
    bool   bIsDirectory = false;
};

struct CORE_API IPlatformFileSystem
{
    /** @brief Unimplemented: declared here, but never defined and never overridden by a platform */
    static void ObtainRelativePath(const String& Path);

    /**
     * @brief Open an existing file for reading, leaving it open to other readers
     * @param Filename Path of the file to open
     * @return Returns a handle the caller closes, or nullptr if the file could not be opened
     */
    static FORCEINLINE IPlatformFile* OpenForRead(const String& Filename)
    {
        return nullptr;
    }

    /**
     * @brief Open a file for writing, creating it if it is missing and locking out other writers
     * @param Filename Path of the file to open
     * @param bTruncate Discard the existing contents instead of writing over them
     * @return Returns a handle the caller closes, or nullptr if the file could not be opened
     */
    static FORCEINLINE IPlatformFile* OpenForWrite(const String& Filename, bool bTruncate = true)
    {
        return nullptr;
    }

    /**
     * @brief Open a file for writing where each write is submitted without blocking the caller
     * @param Filename Path of the file to open
     * @param bTruncate Discard the existing contents instead of writing over them
     * @return Returns a handle the caller closes, or nullptr if the file could not be opened
     */
    static FORCEINLINE IPlatformAsyncFile* OpenForAsyncWrite(const String& Filename, bool bTruncate = true)
    {
        return nullptr;
    }

    /** @return Returns the working directory of the process, or an empty string if it could not be queried */
    static FORCEINLINE String GetCurrentWorkingDirectory()
    {
        return String();
    }

    /** @return Returns the full path of the running executable */
    static FORCEINLINE const CHAR* GetExecutablePath()
    {
        return "";
    }

    /** @return Returns true if Path names an existing directory */
    static FORCEINLINE bool IsDirectory(const CHAR* Path)
    {
        return false;
    }

    /**
     * @brief Check that a path exists
     * @return Returns true if Path exists, which both platforms also answer for a directory
     */
    static FORCEINLINE bool IsFile(const CHAR* Path)
    {
        return false;
    }

    /** @brief Create a directory, returning true if it was created or already existed */
    static FORCEINLINE bool CreateDirectory(const CHAR* Path)
    {
        return false;
    }

    /** @brief Remove a directory, returning true if it was removed or was never there */
    static FORCEINLINE bool RemoveDirectory(const CHAR* Path)
    {
        return false;
    }

    /** @brief Delete a file, returning true if it was deleted or was never there */
    static FORCEINLINE bool DeleteFile(const CHAR* Path)
    {
        return false;
    }

    /** @brief Move a file, replacing ToFilename if it exists, returning true on success */
    static FORCEINLINE bool MoveFile(const CHAR* FromFilename, const CHAR* ToFilename)
    {
        return false;
    }

    /** @return Returns true if Path is relative rather than absolute */
    static FORCEINLINE bool IsPathRelative(const CHAR* Path)
    {
        return false;
    }

    /**
     * @brief Fill OutEntries with the immediate children of Path (not recursive)
     * @return Returns false if Path cannot be opened as a directory
     */
    static FORCEINLINE bool IterateDirectory(const CHAR* Path, TArray<FDirectoryEntry>& OutEntries)
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
