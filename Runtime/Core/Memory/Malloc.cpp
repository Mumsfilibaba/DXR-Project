#include "Malloc.h"

#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Threading/ScopedLock.h"

#if DEBUG_BUILD
    #define USE_DEBUG_MALLOC (0)
#else
    #define USE_DEBUG_MALLOC (0)
#endif

#if DEBUG_BUILD
    #define TRACK_MALLOC_CALLSTACK (0)
#else
    #define TRACK_MALLOC_CALLSTACK (0)
#endif

FMalloc* FMalloc::GInstance = nullptr;

void FMalloc::CreateMalloc()
{
    CHECK(GInstance == nullptr);

    if (!GInstance)
    {
        GInstance = new FMalloc();
        if CONSTEXPR (USE_DEBUG_MALLOC)
        {
            GInstance = new FMallocLeakTracker(GInstance);
        }
        else if CONSTEXPR (TRACK_MALLOC_CALLSTACK)
        {
            GInstance = new FMallocStackTraceTracker(GInstance);
        }
    }

    CHECK(GInstance != nullptr);
}

FMalloc& FMalloc::Get()
{
    if (!GInstance)
    {
        CreateMalloc();
        CHECK(GInstance != nullptr);
    }

    return *GInstance;
}

void* FMalloc::Malloc(uint64 InSize)
{
    return ::malloc(InSize);
}

void* FMalloc::Realloc(void* Block, uint64 InSize)
{
    return ::realloc(Block, InSize);
}

void FMalloc::Free(void* Block)
{
    ::free(Block);
}


FMallocLeakTracker::FMallocLeakTracker(FMalloc* InBaseMalloc)
    : BaseMalloc(InBaseMalloc)
    , Head(nullptr)
    , Tail(nullptr)
    , bTrackingEnabled(true)
{ }

void* FMallocLeakTracker::Malloc(uint64 InSize)
{
    FMemoryHeader* Block = reinterpret_cast<FMemoryHeader*>(BaseMalloc->Malloc(RealSize(InSize)));
    if (!Block)
    {
        return nullptr;
    }

    Block->Next = nullptr;
    Block->Previous = nullptr;
    Block->Size = InSize;

    if (bTrackingEnabled)
    {
        AppendBlock(Block);
    }

    return Block->GetData();
}

void* FMallocLeakTracker::Realloc(void* InBlock, uint64 InSize)
{
    if (InBlock == nullptr)
    {
        return Malloc(InSize);
    }

    void* RealBlock = RetrieveRealPointer(InBlock);
    FMemoryHeader* Block = reinterpret_cast<FMemoryHeader*>(RealBlock);
    RemoveBlock(Block);

    Block = reinterpret_cast<FMemoryHeader*>(BaseMalloc->Realloc(RealBlock, RealSize(InSize)));
    if (!Block)
    {
        return nullptr;
    }

    Block->Next = nullptr;
    Block->Previous = nullptr;
    Block->Size = InSize;

    if (bTrackingEnabled)
    {
        AppendBlock(Block);
    }

    return Block->GetData();
}

void FMallocLeakTracker::Free(void* InBlock)
{
    if (InBlock)
    {
        void* RealBlock = RetrieveRealPointer(InBlock);
        FMemoryHeader* Block = reinterpret_cast<FMemoryHeader*>(RealBlock);
        RemoveBlock(Block);

        BaseMalloc->Free(RealBlock);
    }
}

void FMallocLeakTracker::DumpAllocations(FOutputDevice* OutputDevice)
{
    DisableTacking();

    {
        SCOPED_LOCK(CriticalSection);

        uint32 NumAllocations = 0;
        {
            FMemoryHeader* Current = Head;
            while (Current)
            {
                Current = Current->Next;
                NumAllocations++;
            }
        }

        OutputDevice->Log(FString::CreateFormatted("Current Allocations (Num=%u):", NumAllocations));

        FMemoryHeader* Current = Head;
        while (Current)
        {
            OutputDevice->Log(FString::CreateFormatted(
                "    Address=0x%p Size=%llu",
                Current->GetData(),
                Current->Size));

            Current = Current->Next;
        }
    }

    EnableTracking();
}

void FMallocLeakTracker::AppendBlock(FMemoryHeader* Block)
{
    SCOPED_LOCK(CriticalSection);

    if (Tail)
    {
        Tail->Next = Block;
        Block->Previous = Tail;
        Tail = Block;
    }
    else
    {
        Block->Previous = nullptr;
        Head = Tail = Block;
    }
}

void FMallocLeakTracker::RemoveBlock(FMemoryHeader* Block)
{
    SCOPED_LOCK(CriticalSection);

    FMemoryHeader* Previous = Block->Previous;
    if (Previous)
    {
        Previous->Next  = Block->Next;
        Block->Previous = nullptr;
    }

    FMemoryHeader* Next = Block->Next;
    if (Next)
    {
        Next->Previous = Previous;
        Block->Next    = nullptr;
    }

    if (Block == Head)
    {
        Head = Next;
    }

    if (Block == Tail)
    {
        Tail = Previous;
    }
}


FMallocStackTraceTracker::FMallocStackTraceTracker(FMalloc* InBaseMalloc)
    : BaseMalloc(InBaseMalloc)
    , Head(nullptr)
    , Tail(nullptr)
    , bTrackingEnabled(true)
{ }

void* FMallocStackTraceTracker::Malloc(uint64 InSize)
{
    FMemoryHeader* Block = reinterpret_cast<FMemoryHeader*>(BaseMalloc->Malloc(RealSize(InSize)));
    if (!Block)
    {
        return nullptr;
    }

    FMemory::Memzero(Block, sizeof(FMemoryHeader));
    Block->Size = InSize;

    if (bTrackingEnabled)
    {
        DisableTacking();
        Block->StackDepth = FPlatformStackTrace::CaptureStackTrace(Block->StackTrace, NumStackTraces, 2);
        EnableTracking();

        AppendBlock(Block);
    }

    return Block->GetData();
}

void* FMallocStackTraceTracker::Realloc(void* InBlock, uint64 InSize)
{
    if (InBlock == nullptr)
    {
        return Malloc(InSize);
    }

    void* RealBlock = RetrieveRealPointer(InBlock);
    FMemoryHeader* Block = reinterpret_cast<FMemoryHeader*>(RealBlock);
    RemoveBlock(Block);

    Block = reinterpret_cast<FMemoryHeader*>(BaseMalloc->Realloc(RealBlock, RealSize(InSize)));
    if (!Block)
    {
        return nullptr;
    }

    FMemory::Memzero(Block, sizeof(FMemoryHeader));
    Block->Size = InSize;

    if (bTrackingEnabled)
    {
        DisableTacking();
        Block->StackDepth = FPlatformStackTrace::CaptureStackTrace(Block->StackTrace, NumStackTraces, 2);
        EnableTracking();

        AppendBlock(Block);
    }

    return Block->GetData();
}

void FMallocStackTraceTracker::Free(void* InBlock)
{
    if (InBlock)
    {
        void* RealBlock = RetrieveRealPointer(InBlock);
        FMemoryHeader* Block = reinterpret_cast<FMemoryHeader*>(RealBlock);
        RemoveBlock(Block);

        BaseMalloc->Free(RealBlock);
    }
}

void FMallocStackTraceTracker::DumpAllocations(FOutputDevice* OutputDevice)
{
    DisableTacking();

    {
        SCOPED_LOCK(CriticalSection);

        uint32 NumAllocations = 0;
        {
            FMemoryHeader* Current = Head;
            while (Current)
            {
                Current = Current->Next;
                NumAllocations++;
            }
        }

        OutputDevice->Log(FString::CreateFormatted("Current Allocations (Num=%u):", NumAllocations));

        FPlatformStackTrace::InitializeSymbols();

        FMemoryHeader* Current = Head;
        while (Current)
        {
            FString Message = FString::CreateFormatted(
                "    Address=0x%p Size=%llu\n"
                "        Callstack:\n",
                Current->GetData(),
                Current->Size);

            // Skip first since this is the FPlatformStackTrace::CaptureStackTrace
            for (uint64 Depth = 0; Depth < Current->StackDepth; ++Depth)
            {
                FStackTraceEntry Entry;
                FMemory::Memzero(&Entry);

                FPlatformStackTrace::GetStackTraceEntryFromAddress(Current->StackTrace[Depth], Entry);
                Message.AppendFormat("        %d | %s | %s | %s\n", Entry.Line, Entry.FunctionName, Entry.Filename, Entry.ModuleName);
            }

            OutputDevice->Log(Message);

            Current = Current->Next;
        }

        FPlatformStackTrace::ReleaseSymbols();
    }

    EnableTracking();
}

void FMallocStackTraceTracker::AppendBlock(FMemoryHeader* Block)
{
    SCOPED_LOCK(CriticalSection);

    if (Tail)
    {
        Tail->Next = Block;
        Block->Previous = Tail;
        Tail = Block;
    }
    else
    {
        Block->Previous = nullptr;
        Head = Tail = Block;
    }
}

void FMallocStackTraceTracker::RemoveBlock(FMemoryHeader* Block)
{
    SCOPED_LOCK(CriticalSection);

    FMemoryHeader* Previous = Block->Previous;
    if (Previous)
    {
        Previous->Next  = Block->Next;
        Block->Previous = nullptr;
    }

    FMemoryHeader* Next = Block->Next;
    if (Next)
    {
        Next->Previous = Previous;
        Block->Next    = nullptr;
    }

    if (Block == Head)
    {
        Head = Next;
    }

    if (Block == Tail)
    {
        Tail = Previous;
    }
}
