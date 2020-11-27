#include "New.h"
#include "Memory.h"

Void* operator new(size_t Size)
{
	return Memory::Malloc(Size);
}

Void* operator new[](size_t Size)
{
	return Memory::Malloc(Size);
}

Void* operator new(size_t Size, const std::nothrow_t&) noexcept
{
	return Memory::Malloc(Size);
}

Void* operator new[](size_t Size, const std::nothrow_t&) noexcept
{
	return Memory::Malloc(Size);
}

void operator delete(Void* Ptr) noexcept
{
	Memory::Free(Ptr);
}

void operator delete[](Void* Ptr) noexcept
{
	Memory::Free(Ptr);
}

void operator delete(Void* Ptr, size_t) noexcept
{
	Memory::Free(Ptr);
}

void operator delete[](Void* Ptr, size_t) noexcept
{
	Memory::Free(Ptr);
}