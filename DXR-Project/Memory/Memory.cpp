#include "Memory.h"

#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <crtdbg.h>
#endif

/*
* Memory
*/

Void* Memory::Malloc(UInt64 Size)
{
	return ::malloc(Size);
}

void Memory::Free(Void* Ptr)
{
	::free(Ptr);
}

void Memory::SetDebugFlags(MemoryDebugFlags Flags)
{
#ifdef _WIN32
	UInt32 DebugFlags = 0;
	if (Flags & EMemoryDebugFlag::MEMORY_DEBUG_FLAGS_LEAK_CHECK)
	{
		DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
	}

	_CrtSetDbgFlag(DebugFlags);
#endif
}

Void* Memory::Memset(Void* Destination, UInt8 Value, UInt64 Size)
{
	return ::memset(Destination, static_cast<int>(Value), Size);
}

Void* Memory::Memzero(Void* Destination, UInt64 Size)
{
	return ::memset(Destination, 0, Size);
}

Void* Memory::Memcpy(Void* Destination, const Void* Source, UInt64 Size)
{
	return ::memcpy(Destination, Source, Size);
}

Void* Memory::Memmove(Void* Destination, const Void* Source, UInt64 Size)
{
	return ::memmove(Destination, Source, Size);
}