#include "Memory.h"

#include <cstdlib>
#include <cstring>

#ifdef PLATFORM_WINDOWS
#include <crtdbg.h>
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Memory

void* CMemory::Malloc(uint64 Size) noexcept
{
    // Since malloc is not guaranteed to return nullptr, we check for it here
    // Source: https://www.cplusplus.com/reference/cstdlib/malloc/
    if (Size)
    {
        return malloc(Size);
    }
    else
    {
        return nullptr;
    }
}

void* CMemory::Realloc(void* Pointer, uint64 Size) noexcept
{
    return realloc(Pointer, Size);
}

void CMemory::Free(void* Ptr) noexcept
{
    free(Ptr);
}

void* CMemory::Memset(void* Destination, uint8 Value, uint64 Size) noexcept
{
    return memset(Destination, static_cast<int>(Value), Size);
}

void* CMemory::Memzero(void* Destination, uint64 Size) noexcept
{
    return memset(Destination, 0, Size);
}

void* CMemory::Memcpy(void* restrict_ptr Destination, const void* restrict_ptr Source, uint64 Size) noexcept
{
    return memcpy(Destination, Source, Size);
}

void* CMemory::Memmove(void* Destination, const void* Source, uint64 Size) noexcept
{
    return memmove(Destination, Source, Size);
}

bool CMemory::Memcmp(const void* Lhs, const void* Rhs, uint64 Size)  noexcept
{
    return (memcmp(Lhs, Rhs, Size) == 0);
}

void CMemory::Memswap(void* restrict_ptr Lhs, void* restrict_ptr Rhs, uint64 Size) noexcept
{
    Assert(Lhs != nullptr && Rhs != nullptr);

    // Move 8 bytes at a time 
    uint64* Left = reinterpret_cast<uint64*>(Lhs);
    uint64* Right = reinterpret_cast<uint64*>(Rhs);

    while (Size >= 8)
    {
        uint64 Temp = *Left;
        *Left = *Right;
        *Right = Temp;

        Left++;
        Right++;

        Size -= 8;
    }

    // Move remaining bytes
    uint8* LeftBytes = reinterpret_cast<uint8*>(Lhs);
    uint8* RightBytes = reinterpret_cast<uint8*>(Rhs);

    while (Size)
    {
        uint8 Temp = *LeftBytes;
        *LeftBytes = *RightBytes;
        *RightBytes = Temp;

        LeftBytes++;
        RightBytes++;

        Size--;
    }
}
