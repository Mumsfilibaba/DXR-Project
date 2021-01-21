#include "Memory.h"

#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <crtdbg.h>
#endif

/*
* Memory
*/

void* Memory::Malloc(UInt64 Size)
{
	return ::malloc(Size);
}

void Memory::Free(void* Ptr)
{
	::free(Ptr);
}

Char* Memory::Strcpy(Char* Destination, const Char* Source)
{
	return ::strcpy(Destination, Source);
}

void Memory::SetDebugFlags(MemoryDebugFlags Flags)
{
#ifdef _WIN32
	UInt32 DebugFlags = 0;
	if (Flags & EMemoryDebugFlag::MemoryDebugFlag_LeakCheck)
	{
		DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
	}

	_CrtSetDbgFlag(DebugFlags);
#endif
}

void* Memory::Memset(void* Destination, UInt8 Value, UInt64 Size)
{
	return ::memset(Destination, static_cast<int>(Value), Size);
}

void* Memory::Memzero(void* Destination, UInt64 Size)
{
	return ::memset(Destination, 0, Size);
}

void* Memory::Memcpy(void* Destination, const void* Source, UInt64 Size)
{
	return ::memcpy(Destination, Source, Size);
}

void* Memory::Memmove(void* Destination, const void* Source, UInt64 Size)
{
	return ::memmove(Destination, Source, Size);
}
