#include "Memory.h"

#include <cstdlib>
#include <cstring>
#ifdef _WIN32
	#include <crtdbg.h>
#endif

/*
* Memory
*/

VoidPtr Memory::Malloc(Uint64 Size)
{
	return ::malloc(Size);
}

void Memory::Free(VoidPtr Ptr)
{
	::free(Ptr);
}

void Memory::SetDebugFlags(MemoryDebugFlags Flags)
{
#ifdef _WIN32
	Uint32 DebugFlags = 0;
	if (Flags & EMemoryDebugFlag::MEMORY_DEBUG_FLAGS_LEAK_CHECK)
	{
		DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
	}

	_CrtSetDbgFlag(DebugFlags);
#endif
}

VoidPtr Memory::Memset(VoidPtr Destination, Uint8 Value, Uint64 Size)
{
	return ::memset(Destination, static_cast<int>(Value), Size);
}

VoidPtr Memory::Memzero(VoidPtr Destination, Uint64 Size)
{
	return ::memset(Destination, 0, Size);
}

VoidPtr Memory::Memcpy(VoidPtr Destination, const VoidPtr Source, Uint64 Size)
{
	return ::memcpy(Destination, Source, Size);
}

VoidPtr Memory::Memmove(VoidPtr Destination, const VoidPtr Source, Uint64 Size)
{
	return ::memmove(Destination, Source, Size);
}
