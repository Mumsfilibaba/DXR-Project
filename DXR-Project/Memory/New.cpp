#include "New.h"
#include "Memory.h"

VoidPtr operator new(size_t Size)
{
	return Memory::Malloc(Size);
}

VoidPtr operator new[](size_t Size)
{
	return Memory::Malloc(Size);
}

VoidPtr operator new(size_t Size, const std::nothrow_t&) noexcept
{
	return Memory::Malloc(Size);
}

VoidPtr operator new[](size_t Size, const std::nothrow_t&) noexcept
{
	return Memory::Malloc(Size);
}

void operator delete(VoidPtr Ptr) noexcept
{
	Memory::Free(Ptr);
}

void operator delete[](VoidPtr Ptr) noexcept
{
	Memory::Free(Ptr);
}

void operator delete(VoidPtr Ptr, size_t) noexcept
{
	Memory::Free(Ptr);
}

void operator delete[](VoidPtr Ptr, size_t) noexcept
{
	Memory::Free(Ptr);
}