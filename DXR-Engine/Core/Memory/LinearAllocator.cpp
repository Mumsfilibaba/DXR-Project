#include "LinearAllocator.h"

void* operator new(size_t Size, CLinearAllocator& Allocator)
{
    void* Memory = Allocator.Allocate( Size, 1 );
    Assert( Memory != nullptr );
    return Memory;
}

void* operator new[]( size_t Size, CLinearAllocator& Allocator )
{
    void* Memory = Allocator.Allocate( Size, 1 );
    Assert( Memory != nullptr );
    return Memory;
}

void operator delete (void*, CLinearAllocator&)
{
}

void operator delete[]( void*, CLinearAllocator& )
{
}

CLinearAllocator::CLinearAllocator( uint32 StartSize )
    : CurrentArena( nullptr )
    , Arenas()
{
    CurrentArena = Arenas.Push( MakeUnique<SMemoryArena>(StartSize) ).Get();
}

void* CLinearAllocator::Allocate( uint64 SizeInBytes, uint64 Alignment )
{
    Assert( CurrentArena != nullptr );

    const uint64 AlignedSize = NMath::AlignUp( SizeInBytes, Alignment );
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
    CurrentArena = Arenas.Push( MakeUnique<SMemoryArena>( NewArenaSize ) ).Get();

    Assert( CurrentArena != nullptr );
    return CurrentArena->Allocate( AlignedSize );
}

void CLinearAllocator::Reset()
{
    Assert( CurrentArena != nullptr );
    CurrentArena->Reset();

    if ( Arenas.Size() > 1 )
    {
        Arenas.FirstElement() = Move( Arenas.LastElement() );
        Arenas.Resize( 1 ); // Keep memory for the pointers
        CurrentArena = Arenas.FirstElement().Get();
    }
}
