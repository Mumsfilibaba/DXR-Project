#include "Core/Memory/MemoryProfiler.h"
#include "Core/Algorithms/Sort.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformMemory.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Threading/ScopedLock.h"

CORE_API CONST_INIT FMemoryProfiler GMemoryProfiler;

static CONST_INIT AtomicInt32  GNumLiveThreadStates;
static CONST_INIT AtomicUInt64 GProfilerPageBytes;
static CONST_INIT AtomicUInt64 GProfilerPeakPageBytes;

#if ENABLE_MEMORY_PROFILER
static FAutoConsoleCommand CCmdMemoryDumpTags(
    "Memory.DumpTags",
    "Logs the live and peak bytes of every module tag and memory scope, and the process's memory",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        FMemoryProfiler::Get().LogTags();
    }));

static FAutoConsoleCommand CCmdMemoryDumpLive(
    "Memory.DumpLive",
    "Logs the live allocations of at least MinSize bytes (65536 by default), largest first, with their tags and scopes",
    FConsoleCommandDelegate::CreateLambda([](StringView Args)
    {
        StringView MinSizeView = Args;
        MinSizeView.TrimInline();

        uint64 MinSize = 64 * 1024;
        if (!MinSizeView.IsEmpty())
        {
            const String MinSizeString(MinSizeView);

            CHAR* End = nullptr;
            MinSize = CString::Strtoui64(*MinSizeString, &End, 10);
            if (End == *MinSizeString || *End != '\0')
            {
                LOG_WARNING("[MemoryProfiler] Memory.DumpLive takes a size in bytes, not '%s'", *MinSizeString);
                return;
            }
        }

        FMemoryProfiler::Get().LogLiveAllocations(MinSize);
    }));
#endif

struct alignas(64) FMemoryProfiler::FCounters
{
    AtomicInt64 CurrentBytes;
    AtomicInt64 PeakBytes;
    AtomicInt64 NumAllocations;
    AtomicInt64 TotalAllocations;
};

static String FormatBytes(int64 Bytes)
{
    const int64 Magnitude = Bytes < 0 ? -Bytes : Bytes;
    if (Magnitude >= 1024 * 1024)
    {
        return String::Printf("%.2f MB", static_cast<double>(Bytes) / (1024.0 * 1024.0));
    }

    if (Magnitude >= 1024)
    {
        return String::Printf("%.2f KB", static_cast<double>(Bytes) / 1024.0);
    }

    return String::Printf("%lld B", Bytes);
}

static String GetScopePath(FMemoryTagRegistry& Registry, FMemoryScopeId Scope)
{
    if (Scope == MEMORY_SCOPE_ROOT)
    {
        return "(no scope)";
    }

    String Path;
    for (FMemoryScopeId Node = Scope; Node != MEMORY_SCOPE_ROOT; Node = Registry.GetScopeParent(Node))
    {
        const CHAR* Name = Registry.GetName(Registry.GetScopeName(Node));
        Path = Path.IsEmpty() ? String(Name) : (String(Name) + "/" + Path);
    }

    return Path;
}

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

FMemoryCounterSnapshot FMemoryProfiler::ReadCounters(const FCounters& Counters)
{
    FMemoryCounterSnapshot Snapshot;
    Snapshot.CurrentBytes     = Counters.CurrentBytes.Load(EMemoryOrder::Relaxed);
    Snapshot.PeakBytes        = Counters.PeakBytes.Load(EMemoryOrder::Relaxed);
    Snapshot.NumAllocations   = Counters.NumAllocations.Load(EMemoryOrder::Relaxed);
    Snapshot.TotalAllocations = Counters.TotalAllocations.Load(EMemoryOrder::Relaxed);
    return Snapshot;
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

FMemoryScopeId FMemoryProfiler::SetCurrentScope(FMemoryScopeId Scope)
{
    FMemoryThreadState& State = *ThreadState;

    const FMemoryScopeId PreviousScope = State.ScopeNode;
    State.ScopeNode = Scope;
    return PreviousScope;
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
    if (bCountersReady.Load(EMemoryOrder::Acquire) && Tag < FMemoryTagRegistry::MaxNames)
    {
        Snapshot = ReadCounters(TagCounters[Tag]);
    }

    if (Tag == GetProfilerTag())
    {
        Snapshot.CurrentBytes += static_cast<int64>(MemoryProfilerPages::GetBytes());
        Snapshot.PeakBytes    += static_cast<int64>(MemoryProfilerPages::GetPeakBytes());
    }

    return Snapshot;
}

FMemoryCounterSnapshot FMemoryProfiler::GetScopeStats(FMemoryScopeId Scope)
{
    if (!bCountersReady.Load(EMemoryOrder::Acquire) || Scope >= FMemoryTagRegistry::MaxScopeNodes)
    {
        return FMemoryCounterSnapshot();
    }

    return ReadCounters(ScopeCounters[Scope]);
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

void FMemoryProfiler::Tick()
{
    FMemoryModuleScope ProfilerScope(GetProfilerTag());

    if (!Frames)
    {
        Frames = new TProfilerFrameRing<FMemoryProfilerFrame>();
    }

    FMemoryProfilerFrame Frame;
    Frame.FrameIndex = NextFrameIndex++;
    Frame.TimeStamp  = FPlatformTime::QueryPerformanceCounter();
    FPlatformMemory::GetStats(Frame.PlatformStats);

    if (bCountersReady.Load(EMemoryOrder::Acquire))
    {
        const FMemoryNameId ProfilerTag = GetProfilerTag();
        const int32         NumNames    = Registry.GetNumNames();
        for (int32 Index = 0; Index < NumNames; ++Index)
        {
            const FMemoryNameId          Tag   = static_cast<FMemoryNameId>(Index);
            const FMemoryCounterSnapshot Stats = ReadCounters(TagCounters[Tag]);

            Frame.TrackedBytes          += Stats.CurrentBytes;
            Frame.NumTrackedAllocations += Stats.NumAllocations;

            if (Stats.NumAllocations > 0 || Tag == ProfilerTag)
            {
                FMemoryTagSample& Sample = Frame.Tags.Emplace();
                Sample.Tag            = Tag;
                Sample.CurrentBytes   = Tag == ProfilerTag ? GetTagStats(Tag).CurrentBytes : Stats.CurrentBytes;
                Sample.NumAllocations = Stats.NumAllocations;
            }
        }
    }

    Frames->Push(Move(Frame));
}

void FMemoryProfiler::LogTags()
{
    FMemoryModuleScope ProfilerScope(GetProfilerTag());

    if (!IsTrackingEnabled())
    {
        LOG_INFO("[MemoryProfiler] Allocation tracking is off");
        return;
    }

    struct FRow
    {
        FMemoryNameId          Id;
        FMemoryCounterSnapshot Stats;
    };

    const FMemoryNameId ProfilerTag = GetProfilerTag();

    int64        TrackedBytes = 0;
    TArray<FRow> TagRows;

    const int32 NumNames = Registry.GetNumNames();
    for (int32 Index = 0; Index < NumNames; ++Index)
    {
        const FMemoryNameId          Tag   = static_cast<FMemoryNameId>(Index);
        const FMemoryCounterSnapshot Stats = GetTagStats(Tag);
        if (Stats.PeakBytes > 0)
        {
            TagRows.Add({ Tag, Stats });
        }

        if (bCountersReady.Load(EMemoryOrder::Acquire))
        {
            TrackedBytes += TagCounters[Tag].CurrentBytes.Load(EMemoryOrder::Relaxed);
        }
    }

    Algorithm::Sort(TagRows, [](const FRow& LHS, const FRow& RHS)
    {
        return LHS.Stats.CurrentBytes > RHS.Stats.CurrentBytes;
    });

    FPlatformMemoryStats PlatformStats;
    FPlatformMemory::GetStats(PlatformStats);

    LOG_INFO("[MemoryProfiler] Process: footprint %s (peak %s), resident %s (peak %s)",
        *FormatBytes(static_cast<int64>(PlatformStats.FootprintBytes)), *FormatBytes(static_cast<int64>(PlatformStats.PeakFootprintBytes)),
        *FormatBytes(static_cast<int64>(PlatformStats.ResidentBytes)), *FormatBytes(static_cast<int64>(PlatformStats.PeakResidentBytes)));
    LOG_INFO("[MemoryProfiler] Heap: %s in use, %s of it tracked in %lld allocations",
        *FormatBytes(static_cast<int64>(PlatformStats.AllocatorInUseBytes)), *FormatBytes(TrackedBytes), GetNumTrackedAllocations());

    LOG_INFO("[MemoryProfiler] %-32s %12s %12s %10s %12s", "Tag", "Current", "Peak", "Live", "Total");
    for (const FRow& Row : TagRows)
    {
        LOG_INFO("[MemoryProfiler] %-32s %12s %12s %10lld %12lld%s", Registry.GetName(Row.Id), *FormatBytes(Row.Stats.CurrentBytes),
            *FormatBytes(Row.Stats.PeakBytes), Row.Stats.NumAllocations, Row.Stats.TotalAllocations, Row.Id == ProfilerTag ? " (includes its pages)" : "");
    }

    TArray<FRow> ScopeRows;

    const int32 NumScopeNodes = Registry.GetNumScopeNodes();
    for (int32 Index = 0; Index < NumScopeNodes; ++Index)
    {
        const FMemoryScopeId         Scope = static_cast<FMemoryScopeId>(Index);
        const FMemoryCounterSnapshot Stats = GetScopeStats(Scope);
        if (Scope != MEMORY_SCOPE_ROOT && Stats.PeakBytes > 0)
        {
            ScopeRows.Add({ Scope, Stats });
        }
    }

    if (ScopeRows.IsEmpty())
    {
        return;
    }

    Algorithm::Sort(ScopeRows, [](const FRow& LHS, const FRow& RHS)
    {
        return LHS.Stats.CurrentBytes > RHS.Stats.CurrentBytes;
    });

    LOG_INFO("[MemoryProfiler] %-48s %12s %12s %10s %12s", "Scope (own allocations only)", "Current", "Peak", "Live", "Total");
    for (const FRow& Row : ScopeRows)
    {
        LOG_INFO("[MemoryProfiler] %-48s %12s %12s %10lld %12lld", *GetScopePath(Registry, Row.Id), *FormatBytes(Row.Stats.CurrentBytes),
            *FormatBytes(Row.Stats.PeakBytes), Row.Stats.NumAllocations, Row.Stats.TotalAllocations);
    }
}

void FMemoryProfiler::LogLiveAllocations(uint64 MinSize)
{
    FMemoryModuleScope ProfilerScope(GetProfilerTag());

    if (!IsTrackingEnabled())
    {
        LOG_INFO("[MemoryProfiler] Allocation tracking is off");
        return;
    }

    TArray<FMemoryAllocationEntry> Entries;
    GetLiveAllocations(MinSize, Entries);

    Algorithm::Sort(Entries, [](const FMemoryAllocationEntry& LHS, const FMemoryAllocationEntry& RHS)
    {
        return LHS.Size > RHS.Size;
    });

    int64 TotalBytes = 0;
    for (const FMemoryAllocationEntry& Entry : Entries)
    {
        TotalBytes += static_cast<int64>(Entry.Size);
    }

    LOG_INFO("[MemoryProfiler] %d live allocations of at least %s, %s in total", Entries.Size(), *FormatBytes(static_cast<int64>(MinSize)), *FormatBytes(TotalBytes));
    LOG_INFO("[MemoryProfiler] %12s %-32s %-48s %s", "Size", "Tag", "Scope", "Address");
    for (const FMemoryAllocationEntry& Entry : Entries)
    {
        LOG_INFO("[MemoryProfiler] %12s %-32s %-48s %p", *FormatBytes(static_cast<int64>(Entry.Size)), Registry.GetName(Entry.Tag),
            *GetScopePath(Registry, Entry.Scope), Entry.Block);
    }
}

void FMemoryProfiler::GetLiveAllocations(uint64 MinSize, TArray<FMemoryAllocationEntry>& OutEntries)
{
    // The map copies under its shard locks, so the array is sized first, with room for blocks allocated in between
    const int64 NumEntries = Allocations.CountEntries(MinSize);
    const int64 Capacity   = NumEntries + NumEntries / 8 + 64;

    OutEntries.Reset(static_cast<int32>(Capacity));

    const int64 NumCopied = Allocations.CopyEntries(MinSize, OutEntries.Data(), Capacity);
    OutEntries.Resize(static_cast<int32>(NumCopied));
}

int32 FMemoryProfiler::GetNumFrames()
{
    return Frames ? Frames->Num() : 0;
}

const FMemoryProfilerFrame* FMemoryProfiler::GetFrame(int32 OldestIndex)
{
    return Frames ? Frames->GetOldest(OldestIndex) : nullptr;
}

const FMemoryProfilerFrame* FMemoryProfiler::GetLatestFrame()
{
    return Frames ? Frames->GetLatest() : nullptr;
}
