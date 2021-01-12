#pragma once
#include "Memory.h"

#include <Containers/TArray.h>

/*
* MemoryArena
*/

struct MemoryArena
{
	inline MemoryArena(UInt64 InSizeInBytes)
		: Mem(nullptr)
		, Offset(0)
		, SizeInBytes(InSizeInBytes)
	{
		Mem = reinterpret_cast<Byte*>(Memory::Malloc(SizeInBytes));
		Reset();
	}

	MemoryArena(const MemoryArena& Other) = delete;

	inline MemoryArena(MemoryArena&& Other)
		: Mem(Other.Mem)
		, Offset(Other.Offset)
		, SizeInBytes(Other.SizeInBytes)
	{
		Other.Mem			= nullptr;
		Other.Offset		= 0;
		Other.SizeInBytes	= 0;
	}

	inline ~MemoryArena()
	{
		Memory::Free(Mem);
	}

	FORCEINLINE Void* MemoryArena::Allocate(UInt64 InSizeInBytes)
	{
		VALIDATE(ReservedSize() > InSizeInBytes);

		Void* Allocated = reinterpret_cast<Void*>(Mem + Offset);
		Offset += InSizeInBytes;
		return Allocated;
	}

	FORCEINLINE UInt64 ReservedSize()
	{
		return SizeInBytes - Offset;
	}

	FORCEINLINE void Reset()
	{
		Offset = 0;
	}

	FORCEINLINE UInt64 GetSizeInBytes()
	{
		return SizeInBytes;
	}

	MemoryArena& operator=(const MemoryArena& Other) = delete;

	FORCEINLINE MemoryArena& operator=(MemoryArena&& Other)
	{
		if (Mem)
		{
			Memory::Free(Mem);
		}

		Mem			= Other.Mem;
		Offset		= Other.Offset;
		SizeInBytes = Other.SizeInBytes;

		Other.Mem			= nullptr;
		Other.Offset		= 0;
		Other.SizeInBytes	= 0;

		return *this;
	}

	Byte*	Mem;
	UInt64	Offset;
	UInt64	SizeInBytes;
};

/*
* LinearAllocator
*/

class LinearAllocator
{
public:
	LinearAllocator(UInt32 StartSize = 4096);
	~LinearAllocator() = default;

	Void* Allocate(UInt64 SizeInBytes, UInt64 Alignment);
	
	void Reset();

	template<typename T>
	FORCEINLINE Void* Allocate()
	{
		return Allocate(sizeof(T), alignof(T));
	}

	FORCEINLINE Byte* AllocateBytes(UInt64 SizeInBytes, UInt64 Alignment)
	{
		return reinterpret_cast<Byte*>(Allocate(SizeInBytes, Alignment));
	}

private:
	MemoryArena* CurrentArena;
	TArray<MemoryArena> Arenas;
};