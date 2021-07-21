#pragma once
#include "Core/Types.h"

#include "Memory/Memory.h"

template<typename T>
class TDefaultAllocator
{
public:
    /* Since we do not store the size of the allocation we cannot copy. TODO: See if this is a better approcach */
    TDefaultAllocator( const TDefaultAllocator& ) = delete;
    TDefaultAllocator& operator=( const TDefaultAllocator& ) = delete;

    FORCEINLINE TDefaultAllocator() noexcept
        : Allocation( nullptr )
    {
    }

    FORCEINLINE TDefaultAllocator( TDefaultAllocator&& Other ) noexcept
        : Allocation( Other.Allocation )
    {
        Other.Allocation = nullptr;
    }

    FORCEINLINE ~TDefaultAllocator()
    {
        Free();
    }

    FORCEINLINE T* AllocateOrRealloc( uint32 Count ) noexcept
    {
        return Memory::Realloc<T>( Count );
    }

    FORCEINLINE void Free() noexcept
    {
        Memory::Free( Allocation );
        Allocation = nullptr;
    }

    FORCEINLINE void MoveFrom( TDefaultAllocator&& Other )
    {
        if ( this != &Other )
        {
            Free();

            Allocation = Other.Allocation;
            Other.Allocation = nullptr;
        }
    }

    FORCEINLINE T* Raw() noexcept
    {
        return Allocation;
    }

    FORCEINLINE const T* Raw() const noexcept
    {
        return Allocation;
    }

    TDefaultAllocator& operator=( TDefaultAllocator&& Other )
    {
        MoveFrom( ::Forward<TDefaultAllocator>( Other ) );
        return *this;
    }

private:
    T* Allocation;
};