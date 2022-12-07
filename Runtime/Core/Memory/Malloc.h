#pragma once
#include "Core/Core.h"
#include "Core/Misc/OutputDevice.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Platform/CriticalSection.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FUseUnmanagedMalloc
{
    // Override the global new
    void* operator new(size_t InSize)
    {
        return ::malloc(InSize);
    }

    // Override the global new
    void* operator new[](size_t InSize)
    {
        return ::malloc(InSize);
    }

    // Override the global delete
    void operator delete(void* Object)
    {
        ::free(Object);
    }

    // Override the global delete
    void operator delete[](void* Object)
    {
        ::free(Object);
    }
};


class CORE_API FMalloc
    : public FUseUnmanagedMalloc
{
public:

    // FMalloc instead of IMalloc since IMalloc is already defined in the WindowsHeaders
    virtual ~FMalloc() = default;

    /**
     * @brief  - Retrieve the global Malloc instance 
     * @return - Returns the global Malloc instance
     */
    static FMalloc& Get();

    /**
     * @brief        - Allocates memory  
     * @param InSize - Number of bytes to allocate
     * @return       - Returns a pointer with allocated memory
     */
    virtual void* Malloc(uint64 InSize);

    /**
     * @brief        - Allocates memory, from an existing block and copies over the old content to the new  
     * @param Block  - The old block to reallocate
     * @param InSize - Number of bytes to allocate
     * @return       - Returns a pointer with allocated memory
     */
    virtual void* Realloc(void* Block, uint64 InSize);

    /**
     * @brief       - Frees a block of memory  
     * @param Block - Block to free
     */
    virtual void Free(void* Block);

    /**
     * @brief              - Dumps information about the current allocations. For the standard implementation no info is available.
     * @param OutputDevice - Device to output the information to
     */
    virtual void DumpAllocations(IOutputDevice* OutputDevice) { }

private:
    static void CreateMalloc();

    static FMalloc* GInstance;
};

class CORE_API FMallocLeakTracker
    : public FMalloc
{
    struct ALIGN_AS(16) FMemoryHeader
    {
        FORCEINLINE void* GetData()
        {
            return reinterpret_cast<uint8*>(this) + sizeof(FMemoryHeader);
        }

        FMemoryHeader* Next;
        FMemoryHeader* Previous;
        
        uint64         Size;
    };

public:
    FMallocLeakTracker(FMalloc* InBaseMalloc);
    ~FMallocLeakTracker() = default;

    virtual void* Malloc(uint64 InSize) override final;

    virtual void* Realloc(void* InBlock, uint64 InSize) override final;

    virtual void Free(void* InBlock) override final;

    virtual void DumpAllocations(IOutputDevice* OutputDevice) override final;

    void EnableTracking() { bTrackingEnabled = true; }
    void DisableTacking() { bTrackingEnabled = false; }

private:
    static FORCEINLINE void* RetrieveRealPointer(void* Block)
    {
        return reinterpret_cast<uint8*>(Block) - sizeof(FMemoryHeader);
    }

    static FORCEINLINE uint64 RealSize(uint64 InSize)
    {
        return InSize + sizeof(FMemoryHeader);
    }

    void AppendBlock(FMemoryHeader* Block);
    void RemoveBlock(FMemoryHeader* Block);

    FCriticalSection CriticalSection;

    FMalloc*       BaseMalloc;
    FMemoryHeader* Head;
    FMemoryHeader* Tail;
    bool           bTrackingEnabled;
};


class CORE_API FMallocStackTraceTracker
    : public FMalloc
{
    enum
    {
        NumStackTraces = 8
    };

    struct ALIGN_AS(16) FMemoryHeader
    {
        FORCEINLINE void* GetData()
        {
            return reinterpret_cast<uint8*>(this) + sizeof(FMemoryHeader);
        }

        FMemoryHeader* Next;
        FMemoryHeader* Previous;

        uint64         StackTrace[NumStackTraces];
        uint64         StackDepth;
        
        uint64         Size;
    };

public:
    FMallocStackTraceTracker(FMalloc* InBaseMalloc);
    ~FMallocStackTraceTracker() = default;

    virtual void* Malloc(uint64 InSize) override final;

    virtual void* Realloc(void* InBlock, uint64 InSize) override final;

    virtual void Free(void* InBlock) override final;

    virtual void DumpAllocations(IOutputDevice* OutputDevice) override final;

    void EnableTracking() { bTrackingEnabled = true; }
    void DisableTacking() { bTrackingEnabled = false; }

private:
    static FORCEINLINE void* RetrieveRealPointer(void* Block)
    {
        return reinterpret_cast<uint8*>(Block) - sizeof(FMemoryHeader);
    }

    static FORCEINLINE uint64 RealSize(uint64 InSize)
    {
        return InSize + sizeof(FMemoryHeader);
    }

    void AppendBlock(FMemoryHeader* Block);
    void RemoveBlock(FMemoryHeader* Block);

    FCriticalSection CriticalSection;

    FMalloc*       BaseMalloc;
    FMemoryHeader* Head;
    FMemoryHeader* Tail;
    bool           bTrackingEnabled;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING