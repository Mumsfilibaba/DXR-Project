#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Stream.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IFileHandle
{
    virtual ~IFileHandle() = default;

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

class FFileHandleRef
{ 
public:
    FORCEINLINE FFileHandleRef()
        : Handle(nullptr)
    {
    }

    FORCEINLINE FFileHandleRef(IFileHandle* InHandle)
        : Handle(InHandle)
    {
    }
    
    FORCEINLINE FFileHandleRef(FFileHandleRef&& Other)
        : Handle(Other.Handle)
    {
        Other.Handle = nullptr;
    }

    FORCEINLINE ~FFileHandleRef()
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

    FORCEINLINE IFileHandle* Get() const
    {
        return Handle;
    }

    FORCEINLINE IFileHandle* operator->() const
    {
        return Handle;
    }

    FORCEINLINE operator bool()
    {
        return IsValid();
    }

    FORCEINLINE FFileHandleRef& operator=(FFileHandleRef&& Other)
    {
        if (this != ::AddressOf(Other))
        {
            Handle = Other.Handle;
            Other.Handle = nullptr;
        }

        return *this;
    }

private:
    IFileHandle* Handle;
};

struct CORE_API FGenericPlatformFile
{
    static void ObtainRelativePath(const FString& Path);

    static FORCEINLINE IFileHandle* OpenForRead(const FString& Filename) 
    {
        return nullptr;
    }

    static FORCEINLINE IFileHandle* OpenForWrite(const FString& Filename, bool bTruncate = true)
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

class CORE_API FFileHelpers
{
public:
    static bool ReadFile(IFileHandle* File, FByteInputStream& OutData);
    static bool ReadFile(IFileHandle* File, TArray<uint8>& OutData);
    static bool ReadTextFile(IFileHandle* File, TArray<CHAR>& OutText);

    static FORCEINLINE bool WriteTextFile(IFileHandle* File, const TArray<CHAR>& Text)
    {
        return WriteTextFile(File, Text.Data(), Text.SizeInBytes());
    }

    static FORCEINLINE bool WriteTextFile(IFileHandle* File, const FString& Text)
    {
        return WriteTextFile(File, Text.Data(), Text.SizeInBytes());
    }

    // Returns the Path to the file (Excluding the filename)
    static FString ExtractFilepath(const FString& Filepath);
    
    // Returns the Filename with the extension (Excluding the rest of the path)
    static FString ExtractFilename(const FString& Filepath);
    
    // Returns the Filename without the extension (Excluding the rest of the path)
    static FString ExtractFilenameWithoutExtension(const FString& Filepath);

private:
    static bool WriteTextFile(IFileHandle* File, const CHAR* Text, uint32 Size);
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
