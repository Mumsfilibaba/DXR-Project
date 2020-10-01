#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EMemoryDebugFlags
*/

typedef Uint32 MemoryDebugFlags;

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
	static VoidPtr Malloc(Uint64 Size);
	static void	Free(VoidPtr Ptr);
	
	static void SetDebugFlags(MemoryDebugFlags Flags);
};