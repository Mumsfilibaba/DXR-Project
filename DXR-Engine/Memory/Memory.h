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
	static void* Malloc(UInt64 Size);
	static void Free(void* Ptr);

	template<typename T>
	static T* Malloc(UInt32 Count)
	{
		return reinterpret_cast<T*>(Malloc(sizeof(T) * Count));
	}

	static void* Memset(void* Destination, UInt8 Value, UInt64 Size);
	
	static void* Memzero(void* Destination, UInt64 Size);
	
	template<typename T>
	static T* Memzero(T* Destination)
	{
		return reinterpret_cast<T*>(Memzero(Destination, sizeof(T)));
	}

	static void* Memcpy(void* Destination, const void* Source, UInt64 Size);

	template<typename T>
	static T* Memcpy(T* Destination, T* Source)
	{
		return reinterpret_cast<T*>(Memcpy(Destination, Source, sizeof(T)));
	}

	static void* Memmove(void* Destination, const void* Source, UInt64 Size);
	
	static Char* Strcpy(Char* Destination, const Char* Source);

	static void SetDebugFlags(MemoryDebugFlags Flags);
};