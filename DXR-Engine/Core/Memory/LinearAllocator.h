#pragma once
#include "Memory.h"

#include "Core/Containers/Array.h"
#include "Core/Templates/IsReallocatable.h"

struct SMemoryArena
{
    SMemoryArena( const SMemoryArena& Other ) = delete;
    SMemoryArena& operator=( const SMemoryArena& Other ) = delete;

    FORCEINLINE SMemoryArena()
        : Mem( nullptr )
        , Offset( 0 )
        , SizeInBytes( 0 )
    {
    }

    FORCEINLINE SMemoryArena( uint64 InSizeInBytes )
        : Mem( nullptr )
        , Offset( 0 )
        , SizeInBytes( InSizeInBytes )
    {
        Mem = reinterpret_cast<uint8*>(CMemory::Malloc( SizeInBytes ));
        Reset();
    }

    FORCEINLINE SMemoryArena( SMemoryArena&& Other )
        : Mem( Other.Mem )
        , Offset( Other.Offset )
        , SizeInBytes( Other.SizeInBytes )
    {
        Other.Mem = nullptr;
        Other.Offset = 0;
        Other.SizeInBytes = 0;
    }

    FORCEINLINE ~SMemoryArena()
    {
        CMemory::Free( Mem );
    }

    FORCEINLINE void* Allocate( uint64 InSizeInBytes )
    {
        Assert( ReservedSize() >= InSizeInBytes );

        void* Allocated = reinterpret_cast<void*>(Mem + Offset);
        Offset += InSizeInBytes;
        return Allocated;
    }

    FORCEINLINE uint64 ReservedSize()
    {
        return SizeInBytes - Offset;
    }

    void Reset()
    {
        Offset = 0;
    }

    FORCEINLINE uint64 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

    FORCEINLINE SMemoryArena& operator=( SMemoryArena&& Other )
    {
        if ( Mem )
        {
            CMemory::Free( Mem );
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
struct TIsReallocatable<SMemoryArena>
{
    enum
    {
        Value = true
    };
};

class CLinearAllocator
{
public:
    CLinearAllocator( uint32 StartSize = 4096 );
    ~CLinearAllocator() = default;

    void* Allocate( uint64 SizeInBytes, uint64 Alignment );

    void Reset();

    template<typename T>
    void* Allocate()
    {
        return Allocate( sizeof( T ), alignof(T) );
    }

    FORCEINLINE uint8* AllocateBytes( uint64 SizeInBytes, uint64 Alignment )
    {
        return reinterpret_cast<uint8*>(Allocate( SizeInBytes, Alignment ));
    }

private:
    SMemoryArena* CurrentArena;
    TArray<TUniquePtr<SMemoryArena>> Arenas;
};

void* operator new  (size_t Size, CLinearAllocator& Allocator);
void* operator new[]( size_t Size, CLinearAllocator& Allocator );
void  operator delete  (void*, CLinearAllocator&);
void  operator delete[]( void*, CLinearAllocator& );
