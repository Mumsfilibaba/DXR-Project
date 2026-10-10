#include "Malloc.h"
#include "Core/Platform/PlatformMemory.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Threading/ScopedLock.h"

void* FMallocANSI::Malloc(uint64 InSize, uint32 InAlignment)
{
    if (!InSize)
    {
        return nullptr;
    }

    return FPlatformMemory::Malloc(InSize, InAlignment);
}

void* FMallocANSI::Realloc(void* Block, uint64 InSize, uint32 InAlignment)
{
    if (!Block)
    {
        return Malloc(InSize, InAlignment);
    }

    return FPlatformMemory::Realloc(Block, InSize, InAlignment);
}

void FMallocANSI::Free(void* Block)
{
    if (Block)
    {
        FPlatformMemory::Free(Block);
    }
}

FMallocLeakTracker::FMallocLeakTracker(FMalloc* InBaseMalloc)
    : Allocations()
    , AllocationsCS()
    , BaseMalloc(InBaseMalloc)
    , bTrackingEnabled(true)
{
}

FMallocLeakTracker::~FMallocLeakTracker()
{
    BaseMalloc = nullptr;
}

void* FMallocLeakTracker::Malloc(uint64 InSize, uint32 InAlignment)
{
    void* Block = BaseMalloc->Malloc(InSize, InAlignment);
    if (bTrackingEnabled)
    {
        TrackAllocationMalloc(Block, InSize);
    }

    return Block;
}

void* FMallocLeakTracker::Realloc(void* InBlock, uint64 InSize, uint32 InAlignment)
{
    if (InBlock == nullptr)
    {
        return Malloc(InSize, InAlignment);
    }

    TrackAllocationFree(InBlock);

    void* Block = BaseMalloc->Realloc(InBlock, InSize, InAlignment);
    if (!Block)
    {
        return nullptr;
    }

    if (bTrackingEnabled)
    {
        TrackAllocationMalloc(Block, InSize);
    }

    return Block;
}

void FMallocLeakTracker::Free(void* InBlock)
{
    if (InBlock)
    {
        TrackAllocationFree(InBlock);
        BaseMalloc->Free(InBlock);
    }
}

void FMallocLeakTracker::DumpAllocations(IOutputDevice* OutputDevice)
{
    DisableTacking();

    {
        SCOPED_LOCK(AllocationsCS);

        const uint32 NumAllocations = static_cast<uint32>(Allocations.Size());
        OutputDevice->Log(String::Printf("Current Allocations (Num=%u):", NumAllocations));

        for (auto CurrentAllocation : Allocations)
        {
            OutputDevice->Log(String::Printf(
                "    Address=0x%p Size=%llu",
                CurrentAllocation.First,
                CurrentAllocation.Second.Size));
        }
    }

    EnableTracking();
}

void FMallocLeakTracker::TrackAllocationMalloc(void* Block, uint64 Size)
{
    DisableTacking();

    SCOPED_LOCK(AllocationsCS);

    CHECK(Allocations.Find(Block) == nullptr);
    Allocations.Add(Block, FAllocationInfo{ Size });

    EnableTracking();
}

void FMallocLeakTracker::TrackAllocationFree(void* Block)
{
    DisableTacking();

    SCOPED_LOCK(AllocationsCS);
    Allocations.Remove(Block);

    EnableTracking();
}

FMallocStackTraceTracker* FMallocStackTraceTracker::StackTraceTracker = nullptr;

void FMallocStackTraceTracker::Enable()
{
    if (StackTraceTracker)
    {
        StackTraceTracker->EnableTracking();
    }
}

void FMallocStackTraceTracker::Disable()
{
    if (StackTraceTracker)
    {
        StackTraceTracker->DisableTacking();
    }
}

FMallocStackTraceTracker::FMallocStackTraceTracker(FMalloc* InBaseMalloc)
    : Allocations()
    , AllocationsCS()
    , BaseMalloc(InBaseMalloc)
    , bTrackingEnabled(true)
{
    if (!StackTraceTracker)
    {
        StackTraceTracker = this;
    }
}

FMallocStackTraceTracker::~FMallocStackTraceTracker()
{
    Allocations.~TMap<void*, FAllocationStackTrace>();
    BaseMalloc = nullptr;

    if (StackTraceTracker == this)
    {
        StackTraceTracker = nullptr;
    }
}

void* FMallocStackTraceTracker::Malloc(uint64 InSize, uint32 InAlignment)
{
    void* Block = BaseMalloc->Malloc(InSize, InAlignment);
    if (bTrackingEnabled.Load())
    {
        TrackAllocationMalloc(Block, InSize);
    }

    return Block;
}

void* FMallocStackTraceTracker::Realloc(void* InBlock, uint64 InSize, uint32 InAlignment)
{
    if (InBlock == nullptr)
    {
        return Malloc(InSize, InAlignment);
    }

    TrackAllocationFree(InBlock);

    void* Block = BaseMalloc->Realloc(InBlock, InSize, InAlignment);
    if (!Block)
    {
        return nullptr;
    }

    if (bTrackingEnabled.Load())
    {
        TrackAllocationMalloc(Block, InSize);
    }

    return Block;
}

void FMallocStackTraceTracker::Free(void* InBlock)
{
    if (InBlock)
    {
        TrackAllocationFree(InBlock);
        BaseMalloc->Free(InBlock);
    }
}

void FMallocStackTraceTracker::DumpAllocations(IOutputDevice* OutputDevice)
{
    DisableTacking();

    {
        SCOPED_LOCK(AllocationsCS);

        const int32 NumAllocations = Allocations.Size();
        OutputDevice->Log(String::Printf("Current Allocations (Num=%d):", NumAllocations));

        FPlatformStackTrace::InitializeSymbols();

        for (auto CurrentAllocation : Allocations)
        {
            String Message = String::Printf(
                "    Address=0x%p Size=%llu\n"
                "        Callstack:\n",
                CurrentAllocation.First,
                CurrentAllocation.Second.Size);

            // Skip first since this is the FPlatformStackTrace::CaptureStackTrace function
            const auto Current = CurrentAllocation.Second;
            for (uint64 Depth = 0; Depth < Current.StackDepth; ++Depth)
            {
                FStackTraceEntry Entry;
                Memory::Memzero(&Entry);

                FPlatformStackTrace::GetStackTraceEntryFromAddress(Current.StackTrace[Depth], Entry);
                Message.AppendPrintf("        %d | %s | %s | %s\n", Entry.Line, Entry.FunctionName, Entry.Filename, Entry.ModuleName);
            }

            OutputDevice->Log(Message);
        }

        FPlatformStackTrace::ReleaseSymbols();
    }

    EnableTracking();
}

void FMallocStackTraceTracker::TrackAllocationMalloc(void* Block, uint64 Size)
{
    DisableTacking();

    SCOPED_LOCK(AllocationsCS);

    CHECK(Allocations.Find(Block) == nullptr);
    
    FAllocationStackTrace& Result = Allocations.Add(Block, FAllocationStackTrace());
    Result.StackDepth = FPlatformStackTrace::CaptureStackTrace(Result.StackTrace, NumStackTraces, 3);
    Result.Size       = Size;

    EnableTracking();
}

void FMallocStackTraceTracker::TrackAllocationFree(void* Block)
{
    DisableTacking();

    SCOPED_LOCK(AllocationsCS);
    Allocations.Remove(Block);

    EnableTracking();
}
