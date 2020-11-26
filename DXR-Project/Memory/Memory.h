#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EMemoryDebugFlags
*/

typedef uint32 MemoryDebugFlags;

enum EMemoryDebugFlag : MemoryDebugFlags
{
	MEMORY_DEBUG_FLAGS_NONE			= 0,
	MEMORY_DEBUG_FLAGS_LEAK_CHECK	= FLAG(1),
};

/*
* Memory
*/

class Memory
{
public:
	static void*	Malloc(uint64 Size);
	static void		Free(void* Ptr);

	static void* Memset(void* Destination, uint8 Value, uint64 Size);
	static void* Memzero(void* Destination, uint64 Size);
	static void* Memcpy(void* Destination, const void* Source, uint64 Size);
	static void* Memmove(void* Destination, const void* Source, uint64 Size);

	static void SetDebugFlags(MemoryDebugFlags Flags);
};