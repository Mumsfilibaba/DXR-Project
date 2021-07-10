#include "LinearAllocator.h"

void* operator new(size_t Size, LinearAllocator& Allocator)
{
    void* Memory = Allocator.Allocate( Size, 1 );
    Assert( Memory != nullptr );
    return Memory;
}

void* operator new[]( size_t Size, LinearAllocator& Allocator )
{
    void* Memory = Allocator.Allocate( Size, 1 );
    Assert( Memory != nullptr );
    return Memory;
}

void operator delete (void*, LinearAllocator&)
{
}

void operator delete[]( void*, LinearAllocator& )
{
}

LinearAllocator::LinearAllocator( uint32 StartSize )
    : CurrentArena( nullptr )
    , Arenas()
{
    CurrentArena = &Arenas.EmplaceBack( StartSize );
}

void* LinearAllocator::Allocate( uint64 SizeInBytes, uint64 Alignment )
{
    Assert( CurrentArena != nullptr );

    const uint64 AlignedSize = Math::AlignUp( SizeInBytes, Alignment );
    if ( CurrentArena->ReservedSize() > AlignedSize )
    {
        return CurrentArena->Allocate( AlignedSize );
    }

    // Size for new arena
    const uint64 CurrentSize = CurrentArena->GetSizeInBytes();
    uint64 NewArenaSize = CurrentSize + CurrentSize;
    if ( NewArenaSize <= AlignedSize )
    {
        NewArenaSize = NewArenaSize + SizeInBytes;
    }

    // Allocate new arena
    CurrentArena = &Arenas.EmplaceBack( NewArenaSize );

    Assert( CurrentArena != nullptr );
    return CurrentArena->Allocate( AlignedSize );
}

void LinearAllocator::Reset()
{
    Assert( CurrentArena != nullptr );
    CurrentArena->Reset();

    if ( Arenas.Size() > 1 )
    {
        Arenas.Front() = ::Move( Arenas.Back() );
        Arenas.Resize( 1 ); // Keep memory for the pointers
        CurrentArena = &Arenas.Front();
    }
}