#include "LinearAllocator.h"

void* operator new(size_t Size, LinearAllocator& Allocator)
{
    void* Memory = Allocator.Allocate(Size, 1);
    VALIDATE(Memory != nullptr);
    return Memory;
}

void* operator new[](size_t Size, LinearAllocator& Allocator)
{
    void* Memory = Allocator.Allocate(Size, 1);
    VALIDATE(Memory != nullptr);
    return Memory;
}

void operator delete (void*, LinearAllocator&)
{
}

void operator delete[](void*, LinearAllocator&)
{
}

LinearAllocator::LinearAllocator(UInt32 StartSize)
    : CurrentArena(nullptr)
    , Arenas()
{
    CurrentArena = &Arenas.EmplaceBack(StartSize);
}

Void* LinearAllocator::Allocate(UInt64 SizeInBytes, UInt64 Alignment)
{
    VALIDATE(CurrentArena != nullptr);

    const UInt64 AlignedSize = Math::AlignUp(SizeInBytes, Alignment);
    if (CurrentArena->ReservedSize() > AlignedSize)
    {
        return CurrentArena->Allocate(AlignedSize);
    }

    // Size for new arena
    const UInt64 CurrentSize = CurrentArena->GetSizeInBytes();
    UInt64 NewArenaSize = CurrentSize + CurrentSize;
    if (NewArenaSize <= AlignedSize)
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
        Arenas.Front() = ::Move(Arenas.Back());
        Arenas.Resize(1); // Keep memory for the pointers
        CurrentArena = &Arenas.Front();
    }

    return;
}