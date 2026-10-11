#pragma once
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
    {
    }

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
    FMemoryTagRegistry                                              Registry;
    TThreadLocal<FMemoryThreadState, FMemoryProfilerStateAllocator> ThreadState;
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
#else
    #define MEMORY_MODULE_SCOPE(Name)
    #define MEMORY_SCOPE(Name)
#endif
