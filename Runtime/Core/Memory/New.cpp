#include "New.h"
#include "Memory.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Memory operators

void* operator new(size_t Size)
{
    return Memory::Malloc(Size);
}

void* operator new[](size_t Size)
{
    return Memory::Malloc(Size);
}

void* operator new(size_t Size, const std::nothrow_t&) noexcept
{
    return Memory::Malloc(Size);
}

void* operator new[](size_t Size, const std::nothrow_t&) noexcept
{
    return Memory::Malloc(Size);
}

void operator delete(void* Ptr) noexcept
{
    Memory::Free(Ptr);
}

void operator delete[](void* Ptr) noexcept
{
    Memory::Free(Ptr);
}

void operator delete(void* Ptr, size_t) noexcept
{
    Memory::Free(Ptr);
}

void operator delete[](void* Ptr, size_t) noexcept
{
    Memory::Free(Ptr);
}