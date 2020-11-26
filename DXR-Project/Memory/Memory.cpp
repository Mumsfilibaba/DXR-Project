#include "Memory.h"

#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <crtdbg.h>
#endif

/*
* Memory
*/

void* Memory::Malloc(uint64 Size)
{
	return ::malloc(Size);
}

void Memory::Free(void* Ptr)
{
	::free(Ptr);
}

void Memory::SetDebugFlags(MemoryDebugFlags Flags)
{
#ifdef _WIN32
	uint32 DebugFlags = 0;
	if (Flags & EMemoryDebugFlag::MEMORY_DEBUG_FLAGS_LEAK_CHECK)
	{
		DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
	}

	_CrtSetDbgFlag(DebugFlags);
#endif
}

void* Memory::Memset(void* Destination, uint8 Value, uint64 Size)
{
	return ::memset(Destination, static_cast<int>(Value), Size);
}

void* Memory::Memzero(void* Destination, uint64 Size)
{
	return ::memset(Destination, 0, Size);
}

void* Memory::Memcpy(void* Destination, const void* Source, uint64 Size)
{
	return ::memcpy(Destination, Source, Size);
}

void* Memory::Memmove(void* Destination, const void* Source, uint64 Size)
{
	return ::memmove(Destination, Source, Size);
}