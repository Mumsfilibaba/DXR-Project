#pragma once
#include "Core.h"

/*
* EMemoryDebugFlags
*/

typedef UInt32 MemoryDebugFlags;

enum EMemoryDebugFlag : MemoryDebugFlags
{
	MemoryDebugFlag_None		= 0,
	MemoryDebugFlag_LeakCheck	= FLAG(1),
};

/*
* Memory
*/

class Memory
{
public:
	static Void* Malloc(UInt64 Size);
	static void Free(Void* Ptr);

	static Void* Memset(Void* Destination, UInt8 Value, UInt64 Size);
	static Void* Memzero(Void* Destination, UInt64 Size);
	static Void* Memcpy(Void* Destination, const Void* Source, UInt64 Size);
	static Void* Memmove(Void* Destination, const Void* Source, UInt64 Size);
	
	template<typename T>
	static Void* Memzero(T* Destination)
	{
		return Memzero(Destination, sizeof(T));
	}

	static void SetDebugFlags(MemoryDebugFlags Flags);
};