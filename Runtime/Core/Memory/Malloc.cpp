#include "Malloc.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Threading/ScopedLock.h"

void* FMallocANSI::Malloc(uint64 InSize)
{
    if (InSize)
    {
        return ::malloc(InSize);
    }

    return nullptr;
}

void* FMallocANSI::Realloc(void* Block, uint64 InSize)
{
    if (Block)
    {
        return ::realloc(Block, InSize);
    }

    return Malloc(InSize);
}

void FMallocANSI::Free(void* Block)
{
    if (Block)
    {
        ::free(Block);
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

void* FMallocLeakTracker::Malloc(uint64 InSize)
{
    void* Block = BaseMalloc->Malloc(InSize);
    if (bTrackingEnabled)
    {
        TrackAllocationMalloc(Block, InSize);
    }

    return Block;
}

void* FMallocLeakTracker::Realloc(void* InBlock, uint64 InSize)
{
    if (InBlock == nullptr)
    {
        return Malloc(InSize);
    }

    TrackAllocationFree(InBlock);

    void* Block = BaseMalloc->Realloc(InBlock, InSize);
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

        const uint32 NumAllocations = static_cast<uint32>(Allocations.size());
        OutputDevice->Log(FString::CreateFormatted("Current Allocations (Num=%u):", NumAllocations));

        for (auto CurrentAllocation : Allocations)
        {
            OutputDevice->Log(FString::CreateFormatted(
                "    Address=0x%p Size=%llu",
                CurrentAllocation.first,
                CurrentAllocation.second.Size));
        }
    }

    EnableTracking();
}

void FMallocLeakTracker::TrackAllocationMalloc(void* Block, uint64 Size)
{
    DisableTacking();

    SCOPED_LOCK(AllocationsCS);

    auto ExistingInfo = Allocations.find(Block);
    CHECK(ExistingInfo == Allocations.end());

    auto Result = Allocations.insert(std::make_pair(Block, FAllocationInfo{ Size }));
    CHECK(Result.second == true);

    EnableTracking();
}

void FMallocLeakTracker::TrackAllocationFree(void* Block)
{
    DisableTacking();

    SCOPED_LOCK(AllocationsCS);

    auto ExistingInfo = Allocations.find(Block);
    if (ExistingInfo != Allocations.end())
    {
        Allocations.erase(ExistingInfo);
    }

    EnableTracking();
}


FMallocStackTraceTracker::FMallocStackTraceTracker(FMalloc* InBaseMalloc)
    : Allocations()
    , AllocationsCS()
    , BaseMalloc(InBaseMalloc)
    , bTrackingEnabled(true)
{
}

FMallocStackTraceTracker::~FMallocStackTraceTracker()
{
    Allocations.~TMap<void*, FAllocationStackTrace>();
    BaseMalloc = nullptr;
}

void* FMallocStackTraceTracker::Malloc(uint64 InSize)
{
    void* Block = BaseMalloc->Malloc(InSize);
    if (bTrackingEnabled)
    {
        TrackAllocationMalloc(Block, InSize);
    }

    return Block;
}

void* FMallocStackTraceTracker::Realloc(void* InBlock, uint64 InSize)
{
    if (InBlock == nullptr)
    {
        return Malloc(InSize);
    }

    TrackAllocationFree(InBlock);

    void* Block = BaseMalloc->Realloc(InBlock, InSize);
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

        const uint32 NumAllocations = static_cast<uint32>(Allocations.size());
        OutputDevice->Log(FString::CreateFormatted("Current Allocations (Num=%u):", NumAllocations));

        FPlatformStackTrace::InitializeSymbols();

        for (auto CurrentAllocation : Allocations)
        {
            FString Message = FString::CreateFormatted(
                "    Address=0x%p Size=%llu\n"
                "        Callstack:\n",
                CurrentAllocation.first,
                CurrentAllocation.second.Size);

            // Skip first since this is the FPlatformStackTrace::CaptureStackTrace
            const auto Current = CurrentAllocation.second;
            for (uint64 Depth = 0; Depth < Current.StackDepth; ++Depth)
            {
                FStackTraceEntry Entry;
                FMemory::Memzero(&Entry);

                FPlatformStackTrace::GetStackTraceEntryFromAddress(Current.StackTrace[Depth], Entry);
                Message.AppendFormat("        %d | %s | %s | %s\n", Entry.Line, Entry.FunctionName, Entry.Filename, Entry.ModuleName);
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

    auto ExistingInfo = Allocations.find(Block);
    CHECK(ExistingInfo == Allocations.end());

    auto Result = Allocations.insert(std::make_pair(Block, FAllocationStackTrace()));
    CHECK(Result.second == true);

    Result.first->second.StackDepth = FPlatformStackTrace::CaptureStackTrace(Result.first->second.StackTrace, NumStackTraces, 3);
    Result.first->second.Size = Size;

    EnableTracking();
}

void FMallocStackTraceTracker::TrackAllocationFree(void* Block)
{
    DisableTacking();

    SCOPED_LOCK(AllocationsCS);

    auto ExistingInfo = Allocations.find(Block);
    if (ExistingInfo != Allocations.end())
    {
        Allocations.erase(ExistingInfo);
    }

    EnableTracking();
}
