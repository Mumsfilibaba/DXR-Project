#include "Core/Memory/MemoryProfiler.h"
#include "Core/Platform/PlatformMemory.h"

static CONST_INIT FMemoryProfiler GMemoryProfiler;
static CONST_INIT AtomicInt32     GNumLiveThreadStates;

FMemoryNameId FMemoryName::Resolve() noexcept
{
    const FMemoryNameId NewId = GMemoryProfiler.GetRegistry().InternStatic(Name);
    Id.Store(NewId, EMemoryOrder::Relaxed);
    return NewId;
}

void* FMemoryProfilerStateAllocator::Allocate(uint64 Size, MAYBE_UNUSED uint32 Alignment)
{
    void* Block = FPlatformMemory::AllocatePages(Size);
    if (Block)
    {
        GNumLiveThreadStates.Increment(EMemoryOrder::Relaxed);
    }

    return Block;
}

void FMemoryProfilerStateAllocator::Free(void* Block, uint64 Size)
{
    FPlatformMemory::FreePages(Block, Size);
    GNumLiveThreadStates.Decrement(EMemoryOrder::Relaxed);
}

int32 FMemoryProfilerStateAllocator::GetNumLiveStates()
{
    return GNumLiveThreadStates.Load(EMemoryOrder::Relaxed);
}

FMemoryProfiler& FMemoryProfiler::Get()
{
    return GMemoryProfiler;
}

void FMemoryProfiler::PushTag(FMemoryNameId Tag)
{
    FMemoryThreadState& State = *ThreadState;
    if (State.TagDepth < FMemoryThreadState::MaxTagDepth)
    {
        State.TagStack[State.TagDepth] = Tag;
    }

    State.TagDepth++;
}

void FMemoryProfiler::PopTag()
{
    FMemoryThreadState* State = ThreadState.TryGet();
    if (State && State->TagDepth > 0)
    {
        State->TagDepth--;
    }
}

FMemoryScopeId FMemoryProfiler::PushScope(FMemoryNameId Name)
{
    FMemoryThreadState& State = *ThreadState;

    const FMemoryScopeId PreviousScope = State.ScopeNode;
    State.ScopeNode = Registry.InternScopeNode(PreviousScope, Name);
    return PreviousScope;
}

void FMemoryProfiler::PopScope(FMemoryScopeId PreviousScope)
{
    if (FMemoryThreadState* State = ThreadState.TryGet())
    {
        State->ScopeNode = PreviousScope;
    }
}

FMemoryNameId FMemoryProfiler::GetCurrentTag()
{
    const FMemoryThreadState* State = ThreadState.TryGet();
    if (!State || State->TagDepth == 0)
    {
        return MEMORY_NAME_UNTAGGED;
    }

    const int32 Top = (State->TagDepth < FMemoryThreadState::MaxTagDepth ? State->TagDepth : FMemoryThreadState::MaxTagDepth) - 1;
    return State->TagStack[Top];
}

FMemoryScopeId FMemoryProfiler::GetCurrentScope()
{
    const FMemoryThreadState* State = ThreadState.TryGet();
    return State ? State->ScopeNode : MEMORY_SCOPE_ROOT;
}

FMemoryNameId FMemoryProfiler::InternDynamic(const CHAR* Name)
{
    return Registry.InternDynamic(Name);
}
