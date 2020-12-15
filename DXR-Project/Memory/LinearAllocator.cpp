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
	VALIDATE(SizeInBytes <= SizePerArena);

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

	VALIDATE(Arenas.IsEmpty() == false);
	Arenas.Front() = Move(Arenas.Back());
	CurrentArena = &Arenas.Front();
	Arenas.Resize(1);
	Arenas.Reserve(2);
}