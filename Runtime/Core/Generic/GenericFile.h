#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif


struct IFileHandle
{
    virtual ~IFileHandle() = default;

    /** @brief - Move the file pointer relative to the beginning of the file */
    virtual bool SeekFromStart(int64 NewPosition) = 0;
    
    /** @brief - Move the file pointer relative to the current pointer of the file */
    virtual bool SeekFromCurrent(int64 NewPosition) = 0;

    /** @brief - Move the file pointer relative to the end of the file */
    virtual bool SeekFromEnd(int64 NewPosition) = 0;

    /** @return - Returns the size of the file */
    virtual int64 Size() const = 0;

    /** @return - Returns the current pointer of the file */
    virtual int64 Tell() const = 0;

    /** @brief - Read from the file */
    virtual int32 Read(uint8* Dst, uint32 BytesToRead) = 0;

    /** @brief - Write to the file */
    virtual int32 Write(const uint8* Src, uint32 BytesToWrite) = 0;

    /** @brief - Truncate the file if the file is currently larger than the new size */
    virtual bool Truncate(int64 NewSize) = 0;

    /** @return - Returns true if the FileHandle is valid */
    virtual bool IsValid() const = 0;

    /** @brief - Closes the FileHandle and deletes this instance */
    virtual void Close() = 0;
};


class FFileHandleRef
{ 
public:
    FORCEINLINE FFileHandleRef()
        : Handle(nullptr)
    { }

    FORCEINLINE FFileHandleRef(IFileHandle* InHandle)
        : Handle(InHandle)
    { }
    
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


struct FGenericFile
{
    static FORCEINLINE IFileHandle* OpenForRead(const FString& Filename) 
    {
        return nullptr;
    }

    static FORCEINLINE IFileHandle* OpenForWrite(const FString& Filename)
    {
        return nullptr;
    }
};


struct FFileHelpers
{
public:
    static bool ReadFile(IFileHandle* File, TArray<uint8>& OutData)
    {
        CHECK(File != nullptr);

        const int64 FileSize = File->Size();
        OutData.Resize(static_cast<int32>(FileSize));

        const int32 ReadBytes = File->Read(reinterpret_cast<uint8*>(OutData.GetData()), static_cast<uint32>(FileSize));
        if (ReadBytes <= 0)
        {
            OutData.Clear(true);
            return false;
        }

        return true;
    }

    static bool ReadTextFile(IFileHandle* File, TArray<CHAR>& OutText)
    {
        CHECK(File != nullptr);

        // Get the filesize and add an extra character for the null-terminator
        const int64 FileSize = File->Size();
        OutText.Resize(static_cast<int32>(FileSize) + 1, '\0');

        const int32 ReadBytes = File->Read(reinterpret_cast<uint8*>(OutText.GetData()), static_cast<uint32>(FileSize));
        if (ReadBytes <= 0)
        {
            OutText.Clear(true);
            return false;
        }

        return true;
    }

    static bool WriteTextFile(IFileHandle* File, const TArray<CHAR>& Text)
    {
        return WriteTextFile(File, Text.GetData(), Text.SizeInBytes());
    }

    static bool WriteTextFile(IFileHandle* File, const FString& Text)
    {
        return WriteTextFile(File, Text.GetData(), Text.SizeInBytes());
    }

private:
    static bool WriteTextFile(IFileHandle* File, const CHAR* Text, uint32 Size)
    {
        CHECK(File != nullptr);

        const int32 WrittenBytes = File->Write(reinterpret_cast<const uint8*>(Text), Size);
        if (WrittenBytes <= 0)
        {
            return false;
        }

        return true;
    }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif