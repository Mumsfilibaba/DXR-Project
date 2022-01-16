#include "New.h"
#include "Memory.h"

void* operator new(size_t Size)
{
    return CMemory::Malloc(Size);
}

void* operator new[](size_t Size)
{
    return CMemory::Malloc(Size);
}

void* operator new(size_t Size, const std::nothrow_t&) noexcept
{
    return CMemory::Malloc(Size);
}

void* operator new[](size_t Size, const std::nothrow_t&) noexcept
{
    return CMemory::Malloc(Size);
}

void operator delete(void* Ptr) noexcept
{
    CMemory::Free(Ptr);
}

void operator delete[](void* Ptr) noexcept
{
    CMemory::Free(Ptr);
}

void operator delete(void* Ptr, size_t) noexcept
{
    CMemory::Free(Ptr);
}

void operator delete[](void* Ptr, size_t) noexcept
{
    CMemory::Free(Ptr);
}