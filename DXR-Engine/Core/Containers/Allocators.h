#pragma once
#include "CoreTypes.h"

#include "Core/Memory/Memory.h"

/* Default allocator that allocates from malloc */
template<typename T>
class TDefaultAllocator
{
public:
    using ElementType = T;
    using SizeType = int32;

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

    /* Allocates memory if needed, uses Memory::Realloc */
    FORCEINLINE ElementType* Allocate( SizeType Count ) noexcept
    {
        Allocation = Memory::Realloc<ElementType>( Allocation, Count );
        return Allocation;
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

    /* Retrive the allocation */
    FORCEINLINE ElementType* Raw() noexcept
    {
        return Allocation;
    }

    /* Retrive the allocation */
    FORCEINLINE const ElementType* Raw() const noexcept
    {
        return Allocation;
    }

    FORCEINLINE TDefaultAllocator& operator=( TDefaultAllocator&& Other )
    {
        MoveFrom( Forward<TDefaultAllocator>( Other ) );
        return *this;
    }

private:
    ElementType* Allocation;
};

/* InlineAllocator allocator that has a small fixed size memory, then allocates from malloc */
template<typename T, const int32 InlineBytes = 32>
class TInlineAllocator
{
public:
    using ElementType = T;
    using SizeType = int32;

    static_assert(InlineBytes > sizeof( void* ), "InlineBytes has to be larger that the size of a void*");

    /* Since we do not store the size of the allocation we cannot copy. TODO: See if this is a better approcach */
    TInlineAllocator( const TInlineAllocator& ) = delete;
    TInlineAllocator& operator=( const TInlineAllocator& ) = delete;

    /* Default constructor */
    FORCEINLINE TInlineAllocator() noexcept
        : Size( 0 )
        , Allocation( nullptr )
    {
    }

    /* Move constructor */
    FORCEINLINE TInlineAllocator( TInlineAllocator&& Other ) noexcept
        : Size( Other.Size )
        , Allocation( nullptr )
    {
        Memory::Memexchange( InlineAllocation, Other.InlineAllocation, InlineBytes );
        Other.Size = 0;
    }

    FORCEINLINE ~TInlineAllocator()
    {
        Free();
    }

    /* Allocates memory if needed, uses Memory::Realloc */
    FORCEINLINE ElementType* Allocate( SizeType Count ) noexcept
    {
        // Free exisiting memory
        Free();

        // Allocate new
        Size = sizeof( ElementType ) * Count;
        if ( IsHeapAllocated() )
        {
            Allocation = Memory::Realloc<ElementType>( Allocation, Count );
            return Allocation;
        }
        else
        {
            return GetInlineAllocation();
        }
    }

    /* Make sure that the allocation is freed */
    FORCEINLINE void Free() noexcept
    {
        if ( IsHeapAllocated() )
        {
            Memory::Free( Allocation );
            Allocation = nullptr;
        }
        else
        {
            Memory::Memzero( InlineAllocation, InlineBytes );
        }

        Size = 0;
    }

    /* Move from one allocator to another */
    FORCEINLINE void MoveFrom( TInlineAllocator&& Other )
    {
        if ( this != &Other )
        {
            Free();

            Memory::Memexchange( InlineAllocation, Other.InlineAllocation, InlineBytes );
            Size = Other.Size;
            Other.Size = 0;
        }
    }

    /* Checks weather there is a valid allocation or not */
    FORCEINLINE bool HasAllocation() const noexcept
    {
        return (Size > 0);
    }

    /* Checks weather there is a heap allocation or not */
    FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return (Size > InlineBytes);
    }

    /* Retrive the size */
    FORCEINLINE SizeType GetSize() const noexcept
    {
        return Size;
    }

    /* Retrive the allocation */
    FORCEINLINE ElementType* Raw() noexcept
    {
        return IsHeapAllocated() ? Allocation : GetInlineAllocation();
    }

    /* Retrive the allocation */
    FORCEINLINE const ElementType* Raw() const noexcept
    {
        return IsHeapAllocated() ? Allocation : GetInlineAllocation();
    }

    /* Move assignment */
    FORCEINLINE TInlineAllocator& operator=( TInlineAllocator&& Other )
    {
        MoveFrom( Forward<TInlineAllocator>( Other ) );
        return *this;
    }

private:

    /* Retrive the inline allocation as pointer */
    FORCEINLINE ElementType* GetInlineAllocation() noexcept
    {
        return reinterpret_cast<ElementType*>(InlineAllocation);
    }

    /* Retrive the inline allocation as pointer */
    FORCEINLINE const ElementType* GetInlineAllocation() const noexcept
    {
        return reinterpret_cast<const ElementType*>(InlineAllocation);
    }

    union
    {
        /* Inline bytes */
        int8 InlineAllocation[InlineBytes];

        /* Dynamic allocation */
        ElementType* Allocation;
    };

    int32 Size;
};