#include "Memory.h"
#include "Malloc.h"
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
#elif RELEASE_BUILD
    #define TRACK_MALLOC_CALLSTACK (0)
#elif PRODUCTION_BUILD
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

void* FMemory::Malloc(uint64 Size) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    return GMalloc->Malloc(Size);
}

void* FMemory::Realloc(void* Block, uint64 Size) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    return GMalloc->Realloc(Block, Size);
}

void FMemory::Free(void* Block) noexcept
{
    if (!GMalloc)
    {
        CreateMalloc();
        CHECK(GMalloc != nullptr);
    }

    return GMalloc->Free(Block);
}

void* FMemory::Memset(void* Dst, uint8 Value, uint64 Size) noexcept
{
    return ::memset(Dst, static_cast<int>(Value), Size);
}

void* FMemory::Memzero(void* Dst, uint64 Size) noexcept
{
    return ::memset(Dst, 0, Size);
}

void* FMemory::Memcpy(void* RESTRICT Dst, const void* RESTRICT Src, uint64 Size) noexcept
{
    return ::memcpy(Dst, Src, Size);
}

void* FMemory::Memmove(void* Dst, const void* Src, uint64 Size) noexcept
{
    return ::memmove(Dst, Src, Size);
}

bool FMemory::Memcmp(const void* LHS, const void* RHS, uint64 Size)  noexcept
{
    return (::memcmp(LHS, RHS, Size) == 0);
}

void FMemory::Memswap(void* RESTRICT LHS, void* RESTRICT RHS, uint64 Size) noexcept
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
