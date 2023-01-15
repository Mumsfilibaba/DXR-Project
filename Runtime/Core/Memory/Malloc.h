#pragma once
#include "Core/Core.h"
#include "Core/Misc/OutputDevice.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/Map.h"

struct FMalloc;

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


struct CORE_API FMalloc
    : public FUseUnmanagedMalloc
{
    // FMalloc instead of IMalloc since IMalloc is already defined in the WindowsHeaders
    virtual ~FMalloc() = default;

    /**
     * @brief        - Allocates memory  
     * @param InSize - Number of bytes to allocate
     * @return       - Returns a pointer with allocated memory
     */
    virtual void* Malloc(uint64 InSize) = 0;

    /**
     * @brief        - Allocates memory, from an existing block and copies over the old content to the new  
     * @param Block  - The old block to reallocate
     * @param InSize - Number of bytes to allocate
     * @return       - Returns a pointer with allocated memory
     */
    virtual void* Realloc(void* Block, uint64 InSize) = 0;

    /**
     * @brief       - Frees a block of memory  
     * @param Block - Block to free
     */
    virtual void Free(void* Block) = 0;

    /**
     * @brief              - Dumps information about the current allocations. For the standard implementation no info is available.
     * @param OutputDevice - Device to output the information to
     */
    virtual void DumpAllocations(IOutputDevice* OutputDevice) { }
};


struct CORE_API FMallocANSI
    : public FMalloc
{
    FMallocANSI() = default;
    ~FMallocANSI() = default;

    virtual void* Malloc(uint64 InSize) override final;

    virtual void* Realloc(void* InBlock, uint64 InSize) override final;

    virtual void Free(void* InBlock) override final;
};


class CORE_API FMallocLeakTracker
    : public FMalloc
{
    struct FAllocationInfo
    {
        uint64 Size;
    };

public:
    FMallocLeakTracker(FMalloc* InBaseMalloc);
    ~FMallocLeakTracker() = default;

    virtual void* Malloc(uint64 InSize) override final;

    virtual void* Realloc(void* InBlock, uint64 InSize) override final;

    virtual void Free(void* InBlock) override final;

    virtual void DumpAllocations(IOutputDevice* OutputDevice) override final;

    void TrackAllocationMalloc(void* Block, uint64 Size);
    void TrackAllocationFree(void* Block);

    void EnableTracking() { bTrackingEnabled = true; }
    void DisableTacking() { bTrackingEnabled = false; }

private:
    TMap<void*, FAllocationInfo> Allocations;
    FCriticalSection             AllocationsCS;

	FMalloc* BaseMalloc;
	bool     bTrackingEnabled;
};


class CORE_API FMallocStackTraceTracker
    : public FMalloc
{
    enum
    {
        NumStackTraces = 8
    };

    struct FAllocationStackTrace
    {
        uint64 StackTrace[NumStackTraces];
        uint64 StackDepth;
        uint64 Size;
    };

public:
    FMallocStackTraceTracker(FMalloc* InBaseMalloc);
    ~FMallocStackTraceTracker() = default;

    virtual void* Malloc(uint64 InSize) override final;

    virtual void* Realloc(void* InBlock, uint64 InSize) override final;

    virtual void Free(void* InBlock) override final;

    virtual void DumpAllocations(IOutputDevice* OutputDevice) override final;

	void TrackAllocationMalloc(void* Block, uint64 Size);
	void TrackAllocationFree(void* Block);

    void EnableTracking() { bTrackingEnabled = true; }
    void DisableTacking() { bTrackingEnabled = false; }

private:
	TMap<void*, FAllocationStackTrace> Allocations;
	FCriticalSection                   AllocationsCS;

	FMalloc* BaseMalloc;
	bool     bTrackingEnabled;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING