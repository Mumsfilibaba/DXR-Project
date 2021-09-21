#pragma once
#include "Memory.h"

#include "Core/Containers/Array.h"
#include "Core/Templates/IsReallocatable.h"

struct MemoryArena
{
    MemoryArena( const MemoryArena& Other ) = delete;
    MemoryArena& operator=( const MemoryArena& Other ) = delete;

    MemoryArena()
        : Mem( nullptr )
        , Offset( 0 )
        , SizeInBytes( 0 )
    {
    }

    MemoryArena( uint64 InSizeInBytes )
        : Mem( nullptr )
        , Offset( 0 )
        , SizeInBytes( InSizeInBytes )
    {
        Mem = reinterpret_cast<uint8*>(Memory::Malloc( SizeInBytes ));
        Reset();
    }

    MemoryArena( MemoryArena&& Other )
        : Mem( Other.Mem )
        , Offset( Other.Offset )
        , SizeInBytes( Other.SizeInBytes )
    {
        Other.Mem = nullptr;
        Other.Offset = 0;
        Other.SizeInBytes = 0;
    }

    ~MemoryArena()
    {
        Memory::Free( Mem );
    }

    void* Allocate( uint64 InSizeInBytes )
    {
        Assert( ReservedSize() >= InSizeInBytes );

        void* Allocated = reinterpret_cast<void*>(Mem + Offset);
        Offset += InSizeInBytes;
        return Allocated;
    }

    uint64 ReservedSize()
    {
        return SizeInBytes - Offset;
    }

    void Reset()
    {
        Offset = 0;
    }

    uint64 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

    MemoryArena& operator=( MemoryArena&& Other )
    {
        if ( Mem )
        {
            Memory::Free( Mem );
        }

        Mem = Other.Mem;
        Offset = Other.Offset;
        SizeInBytes = Other.SizeInBytes;

        Other.Mem = nullptr;
        Other.Offset = 0;
        Other.SizeInBytes = 0;

        return *this;
    }

    uint8* Mem;
    uint64 Offset;
    uint64 SizeInBytes;
};

// This should not be necessary, but the move constructor is not called in TArray,
// it insists of calling the deleted copy constructor, Why?
template<>
struct TIsReallocatable<MemoryArena>
{
    enum
    {
        Value = true
    };
};

class LinearAllocator
{
public:
    LinearAllocator( uint32 StartSize = 4096 );
    ~LinearAllocator() = default;

    void* Allocate( uint64 SizeInBytes, uint64 Alignment );

    void Reset();

    template<typename T>
    void* Allocate()
    {
        return Allocate( sizeof( T ), alignof(T) );
    }

    uint8* AllocateBytes( uint64 SizeInBytes, uint64 Alignment )
    {
        return reinterpret_cast<uint8*>(Allocate( SizeInBytes, Alignment ));
    }

private:
    MemoryArena* CurrentArena;
    TArray<MemoryArena> Arenas;
};

void* operator new  (size_t Size, LinearAllocator& Allocator);
void* operator new[]( size_t Size, LinearAllocator& Allocator );
void  operator delete  (void*, LinearAllocator&);
void  operator delete[]( void*, LinearAllocator& );
