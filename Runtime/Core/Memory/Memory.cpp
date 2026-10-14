#include "Core/Memory/Memory.h"
#include "Core/Memory/MemoryProfiler.h"
#include "Core/Memory/MemoryStats.h"
#include "Core/Memory/Malloc.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/Atomic.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

alignas(FMallocANSI) static uint8 GMallocANSIStorage[sizeof(FMallocANSI)];

enum class EMallocState : int32
{
    Uncreated = 0,
    Creating  = 1,
    Created   = 2,
};

static CONST_INIT TAtomicEnum<EMallocState> GMallocState;

static void CreateMalloc()
{
    if (GMallocState.CompareExchange(EMallocState::Creating, EMallocState::Uncreated))
    {
        GMalloc = ::new (GMallocANSIStorage) FMallocANSI();
        GMallocState.Store(EMallocState::Created, EMemoryOrder::Release);
        return;
    }

    while (GMallocState.Load(EMemoryOrder::Acquire) != EMallocState::Created)
    {
        FPlatformThreadMisc::Pause();
    }
}

static FORCEINLINE uint32 NormalizeAlignment(uint32 Alignment)
{
    CHECK((Alignment & (Alignment - 1)) == 0);
    return Alignment > STANDARD_ALIGNMENT ? Alignment : static_cast<uint32>(STANDARD_ALIGNMENT);
}

void* Memory::Malloc(uint64 Size, uint32 Alignment) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    void* Result = GMalloc->Malloc(Size, NormalizeAlignment(Alignment));
    if (Result)
    {
        STAT_ADD(STAT_Memory_AllocationCount, 1);

        #if ENABLE_MEMORY_PROFILER
        if (GMemoryProfiler.IsTrackingEnabled())
        {
            GMemoryProfiler.OnMalloc(Result, Size);
        }
        #endif
    }

    return Result;
}

void* Memory::Realloc(void* Block, uint64 Size, uint32 Alignment) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    #if ENABLE_MEMORY_PROFILER
    const bool bTracked = GMemoryProfiler.IsTrackingEnabled();

    FMemoryReallocTicket Ticket;
    if (bTracked && Block)
    {
        Ticket = GMemoryProfiler.BeginRealloc(Block);
    }
    #endif

    void* Result = GMalloc->Realloc(Block, Size, NormalizeAlignment(Alignment));
    if (!Block && Result)
    {
        STAT_ADD(STAT_Memory_AllocationCount, 1);
    }

    #if ENABLE_MEMORY_PROFILER
    if (bTracked)
    {
        if (Block)
        {
            GMemoryProfiler.EndRealloc(Ticket, Result, Size);
        }
        else if (Result)
        {
            GMemoryProfiler.OnMalloc(Result, Size);
        }
    }
    #endif

    return Result;
}

void Memory::Free(void* Block) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    if (Block)
    {
        STAT_SUBTRACT(STAT_Memory_AllocationCount, 1);

        #if ENABLE_MEMORY_PROFILER
        if (GMemoryProfiler.IsTrackingEnabled())
        {
            GMemoryProfiler.OnFree(Block);
        }
        #endif
    }

    GMalloc->Free(Block);
}

void Memory::OnOutOfMemory(uint64 Size) noexcept
{
    // The heap has just failed, so the message is formatted on the stack and nothing here may allocate.
    CHAR Message[128];
    ::snprintf(Message, sizeof(Message), "Fatal: Out of memory, failed to allocate %llu bytes", static_cast<unsigned long long>(Size));
    FPlatformMisc::OutputDebugString(Message);

    if (FPlatformMisc::IsDebuggerPresent())
    {
        DEBUG_BREAK();
    }

    ::abort();
}

void* Memory::Memset(void* Dst, uint8 Value, uint64 Size) noexcept
{
    return ::memset(Dst, static_cast<int>(Value), Size);
}

void* Memory::Memzero(void* Dst, uint64 Size) noexcept
{
    return ::memset(Dst, 0, Size);
}

void* Memory::Memcpy(void* RESTRICT Dst, const void* RESTRICT Src, uint64 Size) noexcept
{
    return ::memcpy(Dst, Src, Size);
}

void* Memory::Memmove(void* Dst, const void* Src, uint64 Size) noexcept
{
    return ::memmove(Dst, Src, Size);
}

int32 Memory::Memcmp(const void* LHS, const void* RHS, uint64 Size)  noexcept
{
    return ::memcmp(LHS, RHS, Size);
}

void Memory::Memswap(void* RESTRICT LHS, void* RESTRICT RHS, uint64 Size) noexcept
{
    CHECK(LHS != nullptr && RHS != nullptr);

    // Move 8 bytes at a time 
    uint64* Left  = reinterpret_cast<uint64*>(LHS);
    uint64* Right = reinterpret_cast<uint64*>(RHS);

    while (Size >= 8)
    {
        uint64 Temp = *Left;
        *Left  = *Right;
        *Right = Temp;

        Left++;
        Right++;

        Size -= 8;
    }

    // Move remaining bytes
    uint8* LeftBytes  = reinterpret_cast<uint8*>(LHS);
    uint8* RightBytes = reinterpret_cast<uint8*>(RHS);

    while (Size)
    {
        uint8 Temp  = *LeftBytes;
        *LeftBytes  = *RightBytes;
        *RightBytes = Temp;

        LeftBytes++;
        RightBytes++;

        Size--;
    }
}
