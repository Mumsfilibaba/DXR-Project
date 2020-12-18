#include "LinearAllocator.h"

/*
* LinearAllocator
*/

LinearAllocator::LinearAllocator(Uint32 StartSize)
	: CurrentArena(nullptr)
	, Arenas()
{
	CurrentArena = &Arenas.EmplaceBack(StartSize);
}

VoidPtr LinearAllocator::Allocate(Uint64 SizeInBytes, Uint64 Alignment)
{
	VALIDATE(CurrentArena != nullptr);

	const Uint32 AlignedSize = AlignUp(SizeInBytes, Alignment);
	if (CurrentArena->ReservedSize() > AlignedSize)
	{
		return CurrentArena->Allocate(AlignedSize);
	}

	// Size for new arena
	const Uint64 CurrentSize = CurrentArena->GetSizeInBytes();
	Uint64 NewArenaSize = CurrentSize + CurrentSize;
	if (NewArenaSize < AlignedSize)
	{
		NewArenaSize = NewArenaSize + SizeInBytes;
	}

	// Allocate new arena
	CurrentArena = &Arenas.EmplaceBack(NewArenaSize);

	VALIDATE(CurrentArena != nullptr);
	return CurrentArena->Allocate(AlignedSize);
}

void LinearAllocator::Reset()
{
	VALIDATE(CurrentArena != nullptr);
	CurrentArena->Reset();

	if (Arenas.Size() > 1)
	{
		Arenas.Front() = Move(Arenas.Back());
		Arenas.Resize(1); // Keep memory for the pointers
		CurrentArena = &Arenas.Front();
	}

	return;
}