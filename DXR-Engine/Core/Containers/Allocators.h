#pragma once
#include "CoreTypes.h"

#include "Core/Memory/Memory.h"

/* Default allocator that allocates from malloc */
template<typename T>
class TDefaultAllocator
{
public:
    typedef T ElementType;

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

    FORCEINLINE ElementType* AllocateOrRealloc( uint32 Count ) noexcept
    {
        /* This function handles the same size and does not realloc */
        return Memory::Realloc<ElementType>( Count );
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

    FORCEINLINE ElementType* Raw() noexcept
    {
        return Allocation;
    }

    FORCEINLINE const ElementType* Raw() const noexcept
    {
        return Allocation;
    }

    TDefaultAllocator& operator=( TDefaultAllocator&& Other )
    {
        MoveFrom( ::Forward<TDefaultAllocator>( Other ) );
        return *this;
    }

private:
    ElementType* Allocation;
};