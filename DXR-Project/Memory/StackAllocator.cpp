#include "StackAllocator.h"

/*
* StackAllocator
*/

StackAllocator::StackAllocator(Uint32 InSizePerArena)
	: CurrentArena(nullptr)
	, Arenas()
	, SizePerArena(InSizePerArena)
	, ArenaIndex(0)
{
	CurrentArena = &Arenas.EmplaceBack(SizePerArena);
}

VoidPtr StackAllocator::Allocate(Uint64 SizeInBytes, Uint64 Alignment)
{
	VALIDATE(CurrentArena != nullptr);

	const Uint32 AlignedSize = AlignUp(SizeInBytes, Alignment);
	if (CurrentArena->ReservedSize() > AlignedSize)
	{
		return CurrentArena->Allocate(AlignedSize);
	}

	// Check if reuse is possible
	if (ArenaIndex < (Arenas.Size() - 1))
	{
		CurrentArena = &Arenas[++ArenaIndex];
	}
	else
	{
		CurrentArena = &Arenas.EmplaceBack(SizePerArena);
	}

	VALIDATE(CurrentArena != nullptr);
	return CurrentArena->Allocate(AlignedSize);
}

void StackAllocator::Reset()
{
	VALIDATE(Arenas.IsEmpty() == false);
	Arenas.Front().Reset();

	CurrentArena = &Arenas.Front();
	VALIDATE(CurrentArena != nullptr);
}