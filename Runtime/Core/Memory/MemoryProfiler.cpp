#include "Core/Memory/MemoryProfiler.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformMemory.h"
#include "Core/Threading/ScopedLock.h"

CORE_API CONST_INIT FMemoryProfiler GMemoryProfiler;

static CONST_INIT AtomicInt32  GNumLiveThreadStates;
static CONST_INIT AtomicUInt64 GProfilerPageBytes;
static CONST_INIT AtomicUInt64 GProfilerPeakPageBytes;

struct alignas(64) FMemoryProfiler::FCounters
{
    AtomicInt64 CurrentBytes;
    AtomicInt64 PeakBytes;
    AtomicInt64 NumAllocations;
    AtomicInt64 TotalAllocations;
};

static FMemoryNameId GetTopTag(const FMemoryThreadState& State)
{
    if (State.TagDepth == 0)
    {
        return MEMORY_NAME_UNTAGGED;
    }

    return State.TagStack[Math::Min(State.TagDepth, FMemoryThreadState::MaxTagDepth) - 1];
}

void* MemoryProfilerPages::Allocate(uint64 Size)
{
    void* Pages = FPlatformMemory::AllocatePages(Size);
    if (!Pages)
    {
        return nullptr;
    }

    const uint64 PageSize = FPlatformMemory::GetPageSize();
    const uint64 Mapped   = (Size + PageSize - 1) & ~(PageSize - 1);
    const uint64 Bytes    = GProfilerPageBytes.Add(Mapped, EMemoryOrder::Relaxed);

    uint64 Peak = GProfilerPeakPageBytes.Load(EMemoryOrder::Relaxed);
    while (Bytes > Peak && !GProfilerPeakPageBytes.CompareExchange(Bytes, Peak, EMemoryOrder::Relaxed))
    {
        Peak = GProfilerPeakPageBytes.Load(EMemoryOrder::Relaxed);
    }

    return Pages;
}

void MemoryProfilerPages::Free(void* Pages, uint64 Size)
{
    FPlatformMemory::FreePages(Pages, Size);

    const uint64 PageSize = FPlatformMemory::GetPageSize();
    GProfilerPageBytes.Subtract((Size + PageSize - 1) & ~(PageSize - 1), EMemoryOrder::Relaxed);
}

uint64 MemoryProfilerPages::GetBytes()
{
    return GProfilerPageBytes.Load(EMemoryOrder::Relaxed);
}

uint64 MemoryProfilerPages::GetPeakBytes()
{
    return GProfilerPeakPageBytes.Load(EMemoryOrder::Relaxed);
}

FMemoryNameId FMemoryName::Resolve() noexcept
{
    const FMemoryNameId NewId = GMemoryProfiler.GetRegistry().InternStatic(Name);
    Id.Store(NewId, EMemoryOrder::Relaxed);
    return NewId;
}

void* FMemoryProfilerStateAllocator::Allocate(uint64 Size, MAYBE_UNUSED uint32 Alignment)
{
    void* Block = MemoryProfilerPages::Allocate(Size);
    if (Block)
    {
        GNumLiveThreadStates.Increment(EMemoryOrder::Relaxed);
    }

    return Block;
}

void FMemoryProfilerStateAllocator::Free(void* Block, uint64 Size)
{
    MemoryProfilerPages::Free(Block, Size);
    GNumLiveThreadStates.Decrement(EMemoryOrder::Relaxed);
}

int32 FMemoryProfilerStateAllocator::GetNumLiveStates()
{
    return GNumLiveThreadStates.Load(EMemoryOrder::Relaxed);
}

void FMemoryProfiler::AddToCounters(FCounters& Counters, int64 Size)
{
    const int64 Current = Counters.CurrentBytes.Add(Size, EMemoryOrder::Relaxed);

    int64 Peak = Counters.PeakBytes.Load(EMemoryOrder::Relaxed);
    while (Current > Peak && !Counters.PeakBytes.CompareExchange(Current, Peak, EMemoryOrder::Relaxed))
    {
        Peak = Counters.PeakBytes.Load(EMemoryOrder::Relaxed);
    }

    Counters.NumAllocations.Increment(EMemoryOrder::Relaxed);
    Counters.TotalAllocations.Increment(EMemoryOrder::Relaxed);
}

void FMemoryProfiler::RemoveFromCounters(FCounters& Counters, int64 Size)
{
    Counters.CurrentBytes.Subtract(Size, EMemoryOrder::Relaxed);
    Counters.NumAllocations.Decrement(EMemoryOrder::Relaxed);
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
    return State ? GetTopTag(*State) : MEMORY_NAME_UNTAGGED;
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

bool FMemoryProfiler::EnsureCounters()
{
    if (bCountersReady.Load(EMemoryOrder::Acquire))
    {
        return true;
    }

    SCOPED_LOCK(CountersLock);

    if (!TagCounters)
    {
        TagCounters = static_cast<FCounters*>(MemoryProfilerPages::Allocate(sizeof(FCounters) * FMemoryTagRegistry::MaxNames));
    }

    if (!ScopeCounters)
    {
        ScopeCounters = static_cast<FCounters*>(MemoryProfilerPages::Allocate(sizeof(FCounters) * FMemoryTagRegistry::MaxScopeNodes));
    }

    const bool bReady = TagCounters && ScopeCounters;
    bCountersReady.Store(bReady, EMemoryOrder::Release);
    return bReady;
}

void FMemoryProfiler::SetTrackingEnabled(bool bEnabled)
{
    bTrackingEnabled.Store(bEnabled, EMemoryOrder::Relaxed);
    if (bEnabled)
    {
        return;
    }

    Allocations.Clear();

    SCOPED_LOCK(CountersLock);

    if (TagCounters)
    {
        Memory::Memzero(TagCounters, sizeof(FCounters) * FMemoryTagRegistry::MaxNames);
    }

    if (ScopeCounters)
    {
        Memory::Memzero(ScopeCounters, sizeof(FCounters) * FMemoryTagRegistry::MaxScopeNodes);
    }
}

void FMemoryProfiler::OnMalloc(void* Block, uint64 Size)
{
    FMemoryThreadState& State = *ThreadState;
    if (State.bInProfiler || !EnsureCounters())
    {
        return;
    }

    State.bInProfiler = true;

    FMemoryAllocationEntry Entry;
    Entry.Block = Block;
    Entry.Size  = Size;
    Entry.Tag   = GetTopTag(State);
    Entry.Scope = State.ScopeNode;

    FMemoryAllocationEntry       Replaced;
    const EMemoryMapInsertResult Result = Allocations.Insert(Entry, Replaced);

    if (Result == EMemoryMapInsertResult::Replaced)
    {
        RemoveFromCounters(TagCounters[Replaced.Tag], static_cast<int64>(Replaced.Size));
        RemoveFromCounters(ScopeCounters[Replaced.Scope], static_cast<int64>(Replaced.Size));
    }

    if (Result != EMemoryMapInsertResult::Failed)
    {
        AddToCounters(TagCounters[Entry.Tag], static_cast<int64>(Size));
        AddToCounters(ScopeCounters[Entry.Scope], static_cast<int64>(Size));
    }

    State.bInProfiler = false;
}

void FMemoryProfiler::OnFree(void* Block)
{
    const FMemoryThreadState* State = ThreadState.TryGet();
    if (State && State->bInProfiler)
    {
        return;
    }

    // A block the map does not know was allocated before tracking began, and is left alone
    FMemoryAllocationEntry Entry;
    if (!Allocations.Remove(Block, Entry))
    {
        return;
    }

    RemoveFromCounters(TagCounters[Entry.Tag], static_cast<int64>(Entry.Size));
    RemoveFromCounters(ScopeCounters[Entry.Scope], static_cast<int64>(Entry.Size));
}

FMemoryReallocTicket FMemoryProfiler::BeginRealloc(void* Block)
{
    FMemoryReallocTicket Ticket;

    const FMemoryThreadState* State = ThreadState.TryGet();
    if (State && State->bInProfiler)
    {
        Ticket.bSkipped = true;
        return Ticket;
    }

    Ticket.bTracked = Allocations.Remove(Block, Ticket.Entry);
    return Ticket;
}

void FMemoryProfiler::EndRealloc(const FMemoryReallocTicket& Ticket, void* NewBlock, uint64 NewSize)
{
    if (Ticket.bSkipped)
    {
        return;
    }

    if (!NewBlock && NewSize != 0)
    {
        if (Ticket.bTracked)
        {
            FMemoryAllocationEntry Replaced;
            Allocations.Insert(Ticket.Entry, Replaced);
        }

        return;
    }

    if (Ticket.bTracked)
    {
        RemoveFromCounters(TagCounters[Ticket.Entry.Tag], static_cast<int64>(Ticket.Entry.Size));
        RemoveFromCounters(ScopeCounters[Ticket.Entry.Scope], static_cast<int64>(Ticket.Entry.Size));
    }

    if (NewBlock)
    {
        OnMalloc(NewBlock, NewSize);
    }
}

FMemoryCounterSnapshot FMemoryProfiler::GetTagStats(FMemoryNameId Tag)
{
    FMemoryCounterSnapshot Snapshot;
    if (Tag == GetProfilerTag())
    {
        Snapshot.CurrentBytes = static_cast<int64>(MemoryProfilerPages::GetBytes());
        Snapshot.PeakBytes    = static_cast<int64>(MemoryProfilerPages::GetPeakBytes());
        return Snapshot;
    }

    if (!bCountersReady.Load(EMemoryOrder::Acquire) || Tag >= FMemoryTagRegistry::MaxNames)
    {
        return Snapshot;
    }

    const FCounters& Counters = TagCounters[Tag];
    Snapshot.CurrentBytes     = Counters.CurrentBytes.Load(EMemoryOrder::Relaxed);
    Snapshot.PeakBytes        = Counters.PeakBytes.Load(EMemoryOrder::Relaxed);
    Snapshot.NumAllocations   = Counters.NumAllocations.Load(EMemoryOrder::Relaxed);
    Snapshot.TotalAllocations = Counters.TotalAllocations.Load(EMemoryOrder::Relaxed);
    return Snapshot;
}

FMemoryCounterSnapshot FMemoryProfiler::GetScopeStats(FMemoryScopeId Scope)
{
    FMemoryCounterSnapshot Snapshot;
    if (!bCountersReady.Load(EMemoryOrder::Acquire) || Scope >= FMemoryTagRegistry::MaxScopeNodes)
    {
        return Snapshot;
    }

    const FCounters& Counters = ScopeCounters[Scope];
    Snapshot.CurrentBytes     = Counters.CurrentBytes.Load(EMemoryOrder::Relaxed);
    Snapshot.PeakBytes        = Counters.PeakBytes.Load(EMemoryOrder::Relaxed);
    Snapshot.NumAllocations   = Counters.NumAllocations.Load(EMemoryOrder::Relaxed);
    Snapshot.TotalAllocations = Counters.TotalAllocations.Load(EMemoryOrder::Relaxed);
    return Snapshot;
}

FMemoryNameId FMemoryProfiler::GetProfilerTag()
{
    static CONST_INIT FMemoryName ProfilerName("MemoryProfiler");
    return ProfilerName.GetId();
}

int64 FMemoryProfiler::GetNumTrackedAllocations()
{
    return Allocations.GetNumEntries();
}
