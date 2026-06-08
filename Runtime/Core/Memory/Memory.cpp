#include "Core/Memory/Memory.h"
#include "Core/Memory/MemoryStats.h"
#include "Core/Memory/Malloc.h"
#include "Core/Platform/PlatformStackTrace.h"
#include <cstdlib>
#include <cstring>

// TODO: Add CVar for these
#if DEBUG_BUILD
    #define USE_DEBUG_MALLOC (0)
#else
    #define USE_DEBUG_MALLOC (0)
#endif

#if DEBUG_BUILD
    #define TRACK_MALLOC_CALLSTACK (0)
#elif DEVELOPMENT_BUILD
    #define TRACK_MALLOC_CALLSTACK (0)
#elif RELEASE_BUILD
    #define TRACK_MALLOC_CALLSTACK (0)
#endif

static void CreateMalloc()
{
    CHECK(GMalloc == nullptr);

    if (!GMalloc)
    {
        GMalloc = new FMallocANSI();
        if constexpr(USE_DEBUG_MALLOC)
        {
            GMalloc = new FMallocLeakTracker(GMalloc);
        }
        else if constexpr(TRACK_MALLOC_CALLSTACK)
        {
            GMalloc = new FMallocStackTraceTracker(GMalloc);
        }
    }

    CHECK(GMalloc != nullptr);
}

void* Memory::Malloc(uint64 Size) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    void* Result = GMalloc->Malloc(Size);
    if (Result)
    {
        STAT_ADD(STAT_Memory_AllocationCount, 1);
    }

    return Result;
}

void* Memory::Realloc(void* Block, uint64 Size) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    void* Result = GMalloc->Realloc(Block, Size);
    if (!Block && Result)
    {
        STAT_ADD(STAT_Memory_AllocationCount, 1);
    }

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
    }

    GMalloc->Free(Block);
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
