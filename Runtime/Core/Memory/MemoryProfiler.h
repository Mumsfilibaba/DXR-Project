#pragma once
#include "Core/Memory/MemoryAllocationMap.h"
#include "Core/Memory/MemoryProfilerTypes.h"
#include "Core/Memory/MemoryTagRegistry.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/ThreadLocal.h"

struct FMemoryThreadState
{
    /** @brief How deep module tags nest before further pushes stop changing the current tag */
    static constexpr int32 MaxTagDepth = 32;

    /** @brief The module tags pushed on this thread, innermost last */
    FMemoryNameId TagStack[MaxTagDepth];

    /** @brief How many tags are pushed, which can exceed MaxTagDepth so that every pop still matches its push */
    int32 TagDepth = 0;

    /** @brief The scope node allocations on this thread are attributed to */
    FMemoryScopeId ScopeNode = MEMORY_SCOPE_ROOT;

    /** @brief Set while the profiler itself runs on this thread, so its own allocations are not tracked */
    bool bInProfiler = false;
};

struct CORE_API FMemoryProfilerStateAllocator
{
    /**
     * @brief Maps the storage for one thread's state
     *
     * @param Size Number of bytes the state needs
     * @param Alignment Alignment of the state, which a page always satisfies
     * @return Returns the storage, or nullptr when the pages could not be mapped
     */
    static void* Allocate(uint64 Size, uint32 Alignment);

    /**
     * @brief Unmaps a thread's state once that thread has exited
     *
     * @param Block The storage Allocate returned
     * @param Size The size that was passed to Allocate
     */
    static void Free(void* Block, uint64 Size);

    /** @return Returns how many threads currently hold profiler state */
    static int32 GetNumLiveStates();
};

struct FMemoryReallocTicket
{
    /** @brief The entry taken out of the map for the block being reallocated */
    FMemoryAllocationEntry Entry;

    /** @brief Set when the block was tracked, so Entry holds what it was attributed to */
    bool bTracked = false;

    /** @brief Set when the profiler itself was running on the thread, so the realloc is left untracked */
    bool bSkipped = false;
};

class CORE_API FMemoryProfiler
{
public:
    FMemoryProfiler(const FMemoryProfiler&) = delete;
    FMemoryProfiler& operator=(const FMemoryProfiler&) = delete;

    /** @return Returns the process-wide profiler, which is constant-initialized and so usable before main */
    static FMemoryProfiler& Get();

public:

    /** @brief Creates the profiler with no state, so it can be constant-initialized */
    constexpr FMemoryProfiler() noexcept
        : Registry()
        , ThreadState()
        , Allocations()
        , CountersLock()
        , bCountersReady(false)
        , TagCounters(nullptr)
        , ScopeCounters(nullptr)
        , bTrackingEnabled(ENABLE_MEMORY_PROFILER != 0)
    {
    }

    /** @return Returns whether allocations are tracked, which costs Memory one relaxed load when they are not */
    FORCEINLINE bool IsTrackingEnabled() const
    {
        return bTrackingEnabled.Load(EMemoryOrder::Relaxed);
    }

    /**
     * @brief Turns allocation tracking on or off. Turning it off forgets every tracked block and resets the counters,
     * since a block freed while tracking is off could never be matched again. No other thread may allocate meanwhile.
     *
     * @param bEnabled Whether to track allocations
     */
    void SetTrackingEnabled(bool bEnabled);

    /**
     * @brief Records a block Memory has just allocated, attributing it to the calling thread's tag and scope
     *
     * @param Block The new block
     * @param Size The size that was requested
     */
    void OnMalloc(void* Block, uint64 Size);

    /**
     * @brief Forgets a block Memory is about to free. It has to run before the block is released, or another thread
     * could be handed the same address and record it first.
     *
     * @param Block The block being freed
     */
    void OnFree(void* Block);

    /**
     * @brief Takes a block that is about to be reallocated out of the map, for the same reason OnFree runs first
     *
     * @param Block The block being reallocated
     * @return Returns what EndRealloc needs to settle the block once the realloc is done
     */
    FMemoryReallocTicket BeginRealloc(void* Block);

    /**
     * @brief Settles a realloc. A failed realloc puts the old block back, and a successful one bills the new block to
     * the calling thread's current tag and scope, including a realloc to zero bytes that freed the block.
     *
     * @param Ticket What BeginRealloc returned
     * @param NewBlock The block the realloc returned, nullptr when it failed or freed the block
     * @param NewSize The size that was requested
     */
    void EndRealloc(const FMemoryReallocTicket& Ticket, void* NewBlock, uint64 NewSize);

    /**
     * @brief Reads the counters of a module tag. The profiler's own tag reports the pages it has mapped for itself.
     *
     * @param Tag The tag to read
     * @return Returns the tag's counters, all zero for a tag nothing was allocated under
     */
    FMemoryCounterSnapshot GetTagStats(FMemoryNameId Tag);

    /**
     * @brief Reads the counters of a scope node, which cover allocations made directly in it and not in its children
     *
     * @param Scope The scope node to read
     * @return Returns the node's counters, all zero for a node nothing was allocated in
     */
    FMemoryCounterSnapshot GetScopeStats(FMemoryScopeId Scope);

    /** @return Returns the tag the profiler reports its own overhead under */
    FMemoryNameId GetProfilerTag();

    /** @return Returns how many blocks are tracked */
    int64 GetNumTrackedAllocations();

    /**
     * @brief Makes a module tag the calling thread's current tag until the matching PopTag
     *
     * @param Tag The tag to push
     */
    void PushTag(FMemoryNameId Tag);

    /** @brief Restores the calling thread's tag to the one before the last PushTag */
    void PopTag();

    /**
     * @brief Enters a scope one step below the calling thread's current scope
     *
     * @param Name The name of the step
     * @return Returns the scope that was current before, which the matching PopScope restores
     */
    FMemoryScopeId PushScope(FMemoryNameId Name);

    /**
     * @brief Leaves the scope entered by the matching PushScope
     *
     * @param PreviousScope The scope PushScope returned
     */
    void PopScope(FMemoryScopeId PreviousScope);

    /** @return Returns the calling thread's current module tag, MEMORY_NAME_UNTAGGED when none is pushed */
    FMemoryNameId GetCurrentTag();

    /** @return Returns the calling thread's current scope, MEMORY_SCOPE_ROOT outside every scope */
    FMemoryScopeId GetCurrentScope();

    /**
     * @brief Interns a name built at runtime, such as a module or render graph pass name
     *
     * @param Name The name to intern, which may be released once this returns
     * @return Returns the name's id
     */
    FMemoryNameId InternDynamic(const CHAR* Name);

    /** @return Returns the registry that holds every name and scope path */
    FORCEINLINE FMemoryTagRegistry& GetRegistry()
    {
        return Registry;
    }

private:
    struct FCounters;

    static void AddToCounters(FCounters& Counters, int64 Size);
    static void RemoveFromCounters(FCounters& Counters, int64 Size);

    bool EnsureCounters();

    FMemoryTagRegistry                                              Registry;
    TThreadLocal<FMemoryThreadState, FMemoryProfilerStateAllocator> ThreadState;
    FMemoryAllocationMap                                            Allocations;
    FSpinLock                                                       CountersLock;
    AtomicBool                                                      bCountersReady;
    FCounters*                                                      TagCounters;
    FCounters*                                                      ScopeCounters;
    AtomicBool                                                      bTrackingEnabled;
};

extern CORE_API FMemoryProfiler GMemoryProfiler;

FORCEINLINE FMemoryProfiler& FMemoryProfiler::Get()
{
    return GMemoryProfiler;
}

class FMemoryModuleScope : FNonCopyAndNonMovable
{
public:

    /**
     * @brief Pushes the tag a declared name stands for
     *
     * @param Name The name, which is interned on first use
     */
    explicit FMemoryModuleScope(FMemoryName& Name)
    {
        FMemoryProfiler::Get().PushTag(Name.GetId());
    }

    /**
     * @brief Pushes an already interned tag
     *
     * @param Tag The tag to push
     */
    explicit FMemoryModuleScope(FMemoryNameId Tag)
    {
        FMemoryProfiler::Get().PushTag(Tag);
    }

    ~FMemoryModuleScope()
    {
        FMemoryProfiler::Get().PopTag();
    }
};

class FMemoryScope : FNonCopyAndNonMovable
{
public:

    /**
     * @brief Enters the scope a declared name stands for
     *
     * @param Name The name, which is interned on first use
     */
    explicit FMemoryScope(FMemoryName& Name)
        : PreviousScope(FMemoryProfiler::Get().PushScope(Name.GetId()))
    {
    }

    /**
     * @brief Enters a scope by an already interned name
     *
     * @param Name The name of the scope
     */
    explicit FMemoryScope(FMemoryNameId Name)
        : PreviousScope(FMemoryProfiler::Get().PushScope(Name))
    {
    }

    ~FMemoryScope()
    {
        FMemoryProfiler::Get().PopScope(PreviousScope);
    }

private:
    FMemoryScopeId PreviousScope;
};

#if ENABLE_MEMORY_PROFILER
    #define MEMORY_MODULE_SCOPE(Name) \
        static CONST_INIT FMemoryName STRING_CONCAT(MemoryModuleName_, __LINE__)(Name); \
        FMemoryModuleScope STRING_CONCAT(MemoryModuleScope_, __LINE__)(STRING_CONCAT(MemoryModuleName_, __LINE__))

    #define MEMORY_SCOPE(Name) \
        static CONST_INIT FMemoryName STRING_CONCAT(MemoryScopeName_, __LINE__)(Name); \
        FMemoryScope STRING_CONCAT(MemoryScope_, __LINE__)(STRING_CONCAT(MemoryScopeName_, __LINE__))
#else
    #define MEMORY_MODULE_SCOPE(Name)
    #define MEMORY_SCOPE(Name)
#endif
