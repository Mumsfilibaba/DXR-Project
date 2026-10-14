#pragma once
#include "Core/Core.h"
#include "Core/Threading/Atomic.h"

#ifndef ENABLE_MEMORY_PROFILER
    #if RELEASE_BUILD
        #define ENABLE_MEMORY_PROFILER (0)
    #else
        #define ENABLE_MEMORY_PROFILER (1)
    #endif
#endif

/** @brief Identifies an interned name, which is a module tag or one step of a scope path */
using FMemoryNameId = uint16;

/** @brief Identifies a node in the tree of scope paths */
using FMemoryScopeId = uint16;

/** @brief Identifies an interned callstack */
using FMemoryCallstackId = uint32;

/** @brief The tag of memory allocated outside every MEMORY_MODULE_SCOPE */
constexpr FMemoryNameId MEMORY_NAME_UNTAGGED = 0;

/** @brief The name handed out once the name table is full */
constexpr FMemoryNameId MEMORY_NAME_OVERFLOW = 1;

/** @brief Marks a name that has not been interned yet */
constexpr FMemoryNameId MEMORY_NAME_INVALID = 0xFFFF;

/** @brief The scope of memory allocated outside every MEMORY_SCOPE */
constexpr FMemoryScopeId MEMORY_SCOPE_ROOT = 0;

/** @brief The scope handed out once the scope tree is full */
constexpr FMemoryScopeId MEMORY_SCOPE_OVERFLOW = 1;

/** @brief The callstack of an allocation made while callstacks were not captured */
constexpr FMemoryCallstackId MEMORY_CALLSTACK_NONE = 0;

class CORE_API FMemoryName
{
public:
    FMemoryName(const FMemoryName&) = delete;
    FMemoryName& operator=(const FMemoryName&) = delete;

    /**
     * @brief Creates a name that is interned on first use, so it can be constant-initialized
     *
     * @param InName The name, which must outlive the program, as a string literal does
     */
    explicit constexpr FMemoryName(const CHAR* InName) noexcept
        : Name(InName)
        , Id(MEMORY_NAME_INVALID)
    {
    }

    /** @return Returns the interned id, interning the name on the first call */
    FORCEINLINE FMemoryNameId GetId() noexcept
    {
        const FMemoryNameId CachedId = Id.Load(EMemoryOrder::Relaxed);
        return CachedId != MEMORY_NAME_INVALID ? CachedId : Resolve();
    }

    /** @return Returns the name this was declared with */
    FORCEINLINE const CHAR* GetName() const noexcept
    {
        return Name;
    }

private:
    FMemoryNameId Resolve() noexcept;

    const CHAR*  Name;
    AtomicUInt16 Id;
};

struct CORE_API MemoryProfilerPages
{
    /**
     * @brief Maps zeroed pages for the profiler's own tables, counting them as the profiler's overhead
     *
     * @param Size Number of bytes to map, rounded up to whole pages
     * @return Returns the first page, or nullptr when the pages could not be mapped
     */
    static void* Allocate(uint64 Size);

    /**
     * @brief Unmaps pages taken with Allocate
     *
     * @param Pages The pointer Allocate returned
     * @param Size The size that was passed to Allocate
     */
    static void Free(void* Pages, uint64 Size);

    /** @return Returns the bytes the profiler currently has mapped for itself */
    static uint64 GetBytes();

    /** @return Returns the most bytes the profiler has had mapped for itself at once */
    static uint64 GetPeakBytes();
};

struct FMemoryCounterSnapshot
{
    /** @brief Bytes in the allocations still live */
    int64 CurrentBytes = 0;

    /** @brief The most bytes that were live at once */
    int64 PeakBytes = 0;

    /** @brief How many allocations are still live */
    int64 NumAllocations = 0;

    /** @brief How many allocations were ever made, freed ones included */
    int64 TotalAllocations = 0;
};
