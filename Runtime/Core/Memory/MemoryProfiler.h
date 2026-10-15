#pragma once
#include "Core/Memory/MemoryAllocationMap.h"
#include "Core/Memory/MemoryCallstackTable.h"
#include "Core/Memory/MemoryProfilerTypes.h"
#include "Core/Memory/MemoryTagRegistry.h"
#include "Core/Misc/ProfilerTypes.h"
#include "Core/Platform/PlatformMemory.h"
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

class FMemoryProfiler;

extern CORE_API FMemoryProfiler GMemoryProfiler;

struct FMemoryReallocTicket
{
    /** @brief The entry taken out of the map for the block being reallocated */
    FMemoryAllocationEntry Entry;

    /** @brief Set when the block was tracked, so Entry holds what it was attributed to */
    bool bTracked = false;

    /** @brief Set when the profiler itself was running on the thread, so the realloc is left untracked */
    bool bSkipped = false;
};

struct FMemoryTagSample
{
    /** @brief The tag the sample was taken from */
    FMemoryNameId Tag = MEMORY_NAME_UNTAGGED;

    /** @brief Bytes live under the tag when the frame was sampled */
    int64 CurrentBytes = 0;

    /** @brief How many allocations were live under the tag when the frame was sampled */
    int64 NumAllocations = 0;
};

struct FMemoryProfilerFrame
{
    /** @brief How many frames were sampled before this one */
    int32 FrameIndex = 0;

    /** @brief When the frame was sampled, in performance counter ticks */
    uint64 TimeStamp = 0;

    /** @brief What the operating system reported for the process */
    FPlatformMemoryStats PlatformStats;

    /** @brief Bytes live in every tracked allocation, which leaves out the profiler's own pages */
    int64 TrackedBytes = 0;

    /** @brief How many tracked allocations were live */
    int64 NumTrackedAllocations = 0;

    /** @brief Every tag with live allocations, and the profiler's own tag with its pages included */
    TArray<FMemoryTagSample> Tags;
};

struct FMemoryLeakGroup
{
    /** @brief The tag the allocations were made under */
    FMemoryNameId Tag = MEMORY_NAME_UNTAGGED;

    /** @brief The scope the allocations were made in */
    FMemoryScopeId Scope = MEMORY_SCOPE_ROOT;

    /** @brief The callstack the allocations share, MEMORY_CALLSTACK_NONE when none was captured */
    FMemoryCallstackId CallstackId = MEMORY_CALLSTACK_NONE;

    /** @brief How many allocations are in the group */
    int64 NumAllocations = 0;

    /** @brief Bytes in the group's allocations */
    int64 Bytes = 0;
};

struct FMemoryGPUResourceInfo
{
    /** @brief What kind of resource it is */
    EMemoryGPUResourceKind Kind = EMemoryGPUResourceKind::Unknown;

    /** @brief Whether it is in use or waiting to be destroyed */
    EMemoryGPUResourceState State = EMemoryGPUResourceState::Live;

    /** @brief The memory it holds, in bytes */
    uint64 Size = 0;

    /** @brief The tag that was current where it was created */
    FMemoryNameId Tag = MEMORY_NAME_UNTAGGED;

    /** @brief The scope it was created in */
    FMemoryScopeId Scope = MEMORY_SCOPE_ROOT;

    /** @brief Its debug name, empty until it is named */
    String Name;
};

class CORE_API FMemoryProfiler
{
public:

    /** @return Returns the process-wide profiler, which is constant-initialized and so usable before main */
    static FORCEINLINE FMemoryProfiler& Get()
    {
        return GMemoryProfiler;
    }

public:
    FMemoryProfiler(const FMemoryProfiler&) = delete;
    FMemoryProfiler& operator=(const FMemoryProfiler&) = delete;

    /** @brief Creates the profiler with no state, so it can be constant-initialized */
    constexpr FMemoryProfiler() noexcept
        : Registry()
        , ThreadState()
        , Allocations()
        , Callstacks()
        , CountersLock()
        , bCountersReady(false)
        , TagCounters(nullptr)
        , ScopeCounters(nullptr)
        , Frames(nullptr)
        , NextFrameIndex(0)
        , CurrentMarker(0)
        , GPULock()
        , GPUDomain(nullptr)
        , bCaptureCallstacks(false)
        , bTrackingEnabled(ENABLE_MEMORY_PROFILER != 0)
    {
    }

    /**
     * @brief Turns allocation tracking on or off. Turning it off forgets every tracked block and resets the counters,
     * since a block freed while tracking is off could never be matched again. No other thread may allocate meanwhile.
     *
     * @param bEnabled Whether to track allocations
     */
    void SetTrackingEnabled(bool bEnabled);

    /**
     * @brief Turns callstack capture on when the command line has -MemoryTrackCallstacks. It goes through the
     * Memory.TrackCallstacks variable, so the variable reports what is captured.
     */
    void ConfigureFromCommandLine();

    /**
     * @brief Turns capturing the callstack of every allocation on or off. Blocks allocated while it is off keep no
     * callstack.
     *
     * @param bEnabled Whether to capture callstacks
     */
    void SetCallstackCaptureEnabled(bool bEnabled);

    /**
     * @brief Sets a marker, so later queries can list only the blocks allocated after it
     *
     * @return Returns the new marker, which blocks allocated from now on carry
     */
    uint32 AddMarker();

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
     * @brief Samples the counters and the process's memory into the frame history, which keeps the latest
     * NUM_LIVE_PROFILER_FRAMES frames. It runs once per frame on the game thread, the only thread that reads the history.
     */
    void Tick();

    /**
     * @brief Logs the counters of every tag that has held memory, the scopes that hold memory and the process's memory
     */
    void LogTags();

    /**
     * @brief Logs the live allocations of at least a given size, largest first, with their tags, scopes and the
     * callsites of those whose callstack was captured
     *
     * @param MinSize The smallest allocation logged, in bytes
     * @param SinceMarker Logs only blocks allocated at or after this marker, zero for every block
     */
    void LogLiveAllocations(uint64 MinSize, uint32 SinceMarker);

    /**
     * @brief Copies out the live allocations of at least a given size, in no particular order
     *
     * @param MinSize The smallest allocation copied, in bytes
     * @param SinceMarker Copies only blocks allocated at or after this marker, zero for every block
     * @param OutEntries Receives the allocations. A few blocks allocated by other threads while it runs can be missed.
     */
    void GetLiveAllocations(uint64 MinSize, uint32 SinceMarker, TArray<FMemoryAllocationEntry>& OutEntries);

    /**
     * @brief Groups the live allocations by tag, scope and callstack, leaving out the profiler's own tag and the Static
     * tag, which holds memory that is meant to outlive shutdown
     *
     * @param OutGroups Receives the groups, the most bytes first
     */
    void GetLeaks(TArray<FMemoryLeakGroup>& OutGroups);

    /**
     * @brief Formats leak groups as text, with their callstacks symbolicated where they were captured
     *
     * @param Groups The groups GetLeaks returned
     * @return Returns the report
     */
    String BuildLeakReport(const TArray<FMemoryLeakGroup>& Groups);

    /**
     * @brief Reports the allocations still alive, writing the full report to Profiling/MemoryLeaks_<qpc>.txt and logging
     * a summary with the largest groups
     *
     * @return Returns how many allocations were reported
     */
    int64 DumpLeaks();

    /**
     * @brief Copies out the frames of a captured callstack
     *
     * @param Id The callstack's id
     * @param OutFrames Receives the return addresses, innermost first, and needs room for FMemoryCallstackTable::MaxFrames
     * @return Returns how many frames were copied, zero for MEMORY_CALLSTACK_NONE
     */
    int32 GetCallstack(FMemoryCallstackId Id, uint64* OutFrames);

    /** @return Returns how many frames the history holds */
    int32 GetNumFrames();

    /**
     * @brief Reads a frame from the history
     *
     * @param OldestIndex The frame's position, counted from the oldest frame held
     * @return Returns the frame, or nullptr when OldestIndex is out of range
     */
    const FMemoryProfilerFrame* GetFrame(int32 OldestIndex);

    /** @return Returns the frame sampled last, or nullptr before the first Tick */
    const FMemoryProfilerFrame* GetLatestFrame();

    /**
     * @brief Records a GPU resource that has just been created, billed to the calling thread's tag and scope
     *
     * @param Resource Identifies the resource in the later calls
     * @param Kind What kind of resource it is
     * @param Size The memory it holds, in bytes
     */
    void OnGPUResourceCreated(const void* Resource, EMemoryGPUResourceKind Kind, uint64 Size);

    /**
     * @brief Changes the memory a GPU resource holds, keeping its tag and scope
     *
     * @param Resource The resource
     * @param NewSize The memory it holds now, in bytes
     */
    void OnGPUResourceResized(const void* Resource, uint64 NewSize);

    /**
     * @brief Marks a GPU resource that nothing references any more. Its memory stays counted until it is destroyed,
     * since the GPU can still be using it.
     *
     * @param Resource The resource
     */
    void OnGPUResourceReleased(const void* Resource);

    /**
     * @brief Forgets a GPU resource and gives its memory back. A resource that was never recorded is ignored.
     *
     * @param Resource The resource
     */
    void OnGPUResourceDestroyed(const void* Resource);

    /**
     * @brief Sets the debug name of a GPU resource
     *
     * @param Resource The resource
     * @param Name The name, which is copied
     */
    void OnGPUResourceNamed(const void* Resource, const CHAR* Name);

    /**
     * @brief Looks up a GPU resource
     *
     * @param Resource The resource
     * @param OutInfo Receives what is recorded about it
     * @return Returns false when the resource is not recorded
     */
    bool GetGPUResource(const void* Resource, FMemoryGPUResourceInfo& OutInfo);

    /**
     * @brief Reads the GPU counters of a module tag, which count resources pending deletion as live
     *
     * @param Tag The tag to read
     * @return Returns the tag's counters, all zero for a tag no resource was created under
     */
    FMemoryCounterSnapshot GetGPUTagStats(FMemoryNameId Tag);

    /**
     * @brief Reads the GPU counters of a scope node, which cover resources created directly in it
     *
     * @param Scope The scope node to read
     * @return Returns the node's counters, all zero for a node no resource was created in
     */
    FMemoryCounterSnapshot GetGPUScopeStats(FMemoryScopeId Scope);

    /** @return Returns the bytes held by GPU resources that are released but not destroyed yet */
    int64 GetGPUPendingDeletionBytes();

    /**
     * @brief Reads the counters of a module tag. The profiler's own tag adds the pages it has mapped for itself to its
     * bytes but not to its counts, and its peak is then the sum of two peaks that need not have been reached at once.
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

    /**
     * @brief Makes an existing scope node the calling thread's current scope, which is how work queued on one thread
     * keeps the scope it was queued in when another thread runs it.
     *
     * @param Scope The node to make current
     * @return Returns the scope that was current before, which a later call restores
     */
    FMemoryScopeId SetCurrentScope(FMemoryScopeId Scope);

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

    /** @return Returns whether allocations are tracked, which costs Memory one relaxed load when they are not */
    FORCEINLINE bool IsTrackingEnabled() const
    {
        return bTrackingEnabled.Load(EMemoryOrder::Relaxed);
    }

    /** @return Returns whether the callstack of every allocation is captured */
    FORCEINLINE bool IsCallstackCaptureEnabled() const
    {
        return bCaptureCallstacks.Load(EMemoryOrder::Relaxed);
    }

    /** @return Returns the registry that holds every name and scope path */
    FORCEINLINE FMemoryTagRegistry& GetRegistry()
    {
        return Registry;
    }

private:
    struct FCounters;
    struct FGPUDomain;

    static void AddToCounters(FCounters& Counters, int64 Size);
    static void RemoveFromCounters(FCounters& Counters, int64 Size);
    static FMemoryCounterSnapshot ReadCounters(const FCounters& Counters);

    bool EnsureCounters();

    FMemoryTagRegistry                                              Registry;
    TThreadLocal<FMemoryThreadState, FMemoryProfilerStateAllocator> ThreadState;
    FMemoryAllocationMap                                            Allocations;
    FMemoryCallstackTable                                           Callstacks;
    FSpinLock                                                       CountersLock;
    AtomicBool                                                      bCountersReady;
    FCounters*                                                      TagCounters;
    FCounters*                                                      ScopeCounters;
    TProfilerFrameRing<FMemoryProfilerFrame>*                       Frames;
    int32                                                           NextFrameIndex;
    AtomicUInt32                                                    CurrentMarker;
    FSpinLock                                                       GPULock;
    FGPUDomain*                                                     GPUDomain;
    AtomicBool                                                      bCaptureCallstacks;
    AtomicBool                                                      bTrackingEnabled;
};

class FMemoryContextScope : FNonCopyAndNonMovable
{
public:

    /**
     * @brief Makes a tag and a scope node current for the lifetime of the object, so work runs under the tag and scope
     * of the thread that queued it.
     *
     * @param Tag The tag to push
     * @param Scope The scope node to make current
     */
    FMemoryContextScope(FMemoryNameId Tag, FMemoryScopeId Scope)
        : PreviousScope(FMemoryProfiler::Get().SetCurrentScope(Scope))
    {
        FMemoryProfiler::Get().PushTag(Tag);
    }

    ~FMemoryContextScope()
    {
        FMemoryProfiler::Get().PopTag();
        FMemoryProfiler::Get().SetCurrentScope(PreviousScope);
    }

private:
    FMemoryScopeId PreviousScope;
};

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

    #define MEMORY_MODULE_SCOPE_DYNAMIC(Name) \
        FMemoryModuleScope STRING_CONCAT(MemoryModuleScope_, __LINE__)(FMemoryProfiler::Get().InternDynamic(Name))

    #define MEMORY_MODULE_SCOPE_ID(Tag) \
        FMemoryModuleScope STRING_CONCAT(MemoryModuleScope_, __LINE__)(Tag)
#else
    #define MEMORY_MODULE_SCOPE(Name)
    #define MEMORY_SCOPE(Name)
    #define MEMORY_MODULE_SCOPE_DYNAMIC(Name)
    #define MEMORY_MODULE_SCOPE_ID(Tag)
#endif
