#include "Memory.h"

#include <cstdlib>
#include <cstring>

#ifdef PLATFORM_WINDOWS
#include <crtdbg.h>
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Memory

void* FMemory::Malloc(uint64 Size) noexcept
{
    // Since malloc is not guaranteed to return nullptr, we check for it here
    // Src: https://www.cplusplus.com/reference/cstdlib/malloc/
    if (Size)
    {
        return malloc(Size);
    }
    else
    {
        return nullptr;
    }
}

void* FMemory::MallocZeroed(uint64 Size) noexcept
{
    void* NewMemory = Malloc(Size);
    Memzero(NewMemory, Size);
    return NewMemory;
}

void* FMemory::Realloc(void* Ptr, uint64 Size) noexcept
{
    return realloc(Ptr, Size);
}

void FMemory::Free(void* Ptr) noexcept
{
    free(Ptr);
}

void* FMemory::Memset(void* Dst, uint8 Value, uint64 Size) noexcept
{
    return memset(Dst, static_cast<int>(Value), Size);
}

void* FMemory::Memzero(void* Dst, uint64 Size) noexcept
{
    return memset(Dst, 0, Size);
}

void* FMemory::Memcpy(void* restrict_ptr Dst, const void* restrict_ptr Src, uint64 Size) noexcept
{
    return memcpy(Dst, Src, Size);
}

void* FMemory::Memmove(void* Dst, const void* Src, uint64 Size) noexcept
{
    return memmove(Dst, Src, Size);
}

bool FMemory::Memcmp(const void* LHS, const void* RHS, uint64 Size)  noexcept
{
    return (memcmp(LHS, RHS, Size) == 0);
}

void FMemory::Memswap(void* restrict_ptr LHS, void* restrict_ptr RHS, uint64 Size) noexcept
{
    Check(LHS != nullptr && RHS != nullptr);

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