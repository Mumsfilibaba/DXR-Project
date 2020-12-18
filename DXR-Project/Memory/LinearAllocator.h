#pragma once
#include "Memory.h"

#include "Containers/TArray.h"

/*
* MemoryArena
*/

struct MemoryArena
{
	inline MemoryArena(Uint64 InSizeInBytes)
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

	FORCEINLINE VoidPtr MemoryArena::Allocate(Uint64 InSizeInBytes)
	{
		VALIDATE(ReservedSize() > InSizeInBytes);

		VoidPtr Allocated = reinterpret_cast<VoidPtr>(Mem + Offset);
		Offset += InSizeInBytes;
		return Allocated;
	}

	FORCEINLINE Uint64 ReservedSize()
	{
		return SizeInBytes - Offset;
	}

	FORCEINLINE void Reset()
	{
		Offset = 0;
	}

	FORCEINLINE Uint64 GetSizeInBytes()
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
	Uint64	Offset;
	Uint64	SizeInBytes;
};

/*
* LinearAllocator
*/

class LinearAllocator
{
public:
	LinearAllocator(Uint32 StartSize = 4096);
	~LinearAllocator() = default;

	VoidPtr Allocate(Uint64 SizeInBytes, Uint64 Alignment);
	
	void Reset();

	template<typename T>
	FORCEINLINE VoidPtr Allocate()
	{
		return Allocate(sizeof(T), alignof(T));
	}

	FORCEINLINE Byte* AllocateBytes(Uint64 SizeInBytes, Uint64 Alignment)
	{
		return reinterpret_cast<Byte*>(Allocate(SizeInBytes, Alignment));
	}

private:
	MemoryArena* CurrentArena;
	TArray<MemoryArena> Arenas;
};