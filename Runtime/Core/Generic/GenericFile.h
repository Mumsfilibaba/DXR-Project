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
    FFileHandleRef()
        : Handle(nullptr)
    { }

    FFileHandleRef(IFileHandle* InHandle)
        : Handle(InHandle)
    { }
    
    FFileHandleRef(FFileHandleRef&& Other)
        : Handle(Other.Handle)
    {
        Other.Handle = nullptr;
    }

    ~FFileHandleRef()
    {
        Close();
    }

    bool IsValid() const
    {
        return (Handle != nullptr);
    }

    void Close()
    {
        if (Handle)
        {
            Handle->Close();
            Handle = nullptr;
        }
    }

    IFileHandle* operator->() const
    {
        return Handle;
    }

    operator bool()
    {
        return IsValid();
    }

    FFileHandleRef& operator=(FFileHandleRef&& Other)
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

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif