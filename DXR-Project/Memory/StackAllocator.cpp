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
	VALIDATE(SizeInBytes <= SizePerArena);

	const Uint32 AlignedSize = AlignUp(SizeInBytes, Alignment);
	if (CurrentArena->ReservedSize() > AlignedSize)
	{
		return CurrentArena->Allocate(AlignedSize);
	}

	// Check if reuse is possible
	const Uint32 LastIndex = Arenas.Size() - 1;
	if (ArenaIndex < LastIndex)
	{
		CurrentArena = &Arenas[++ArenaIndex];
		CurrentArena->Reset();
	}
	else
	{
		CurrentArena = &Arenas.EmplaceBack(SizePerArena);
		ArenaIndex++;
	}

	VALIDATE(CurrentArena != nullptr);
	return CurrentArena->Allocate(AlignedSize);
}

void StackAllocator::Reset()
{
	VALIDATE(Arenas.IsEmpty() == false);
	ArenaIndex		= 0;
	CurrentArena	= &Arenas.Front();

	VALIDATE(CurrentArena != nullptr);
	CurrentArena->Reset();
}