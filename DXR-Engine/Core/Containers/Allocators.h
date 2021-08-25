#pragma once
#include "CoreTypes.h"

#include "Core/Memory/Memory.h"

/* Default allocator that allocates from malloc */
template<typename T>
class TDefaultArrayAllocator
{
public:
    using ElementType = T;
    using SizeType = int32;

    /* Since we do not store the size of the allocation we cannot copy. TODO: See if this is a better approcach */
    TDefaultArrayAllocator( const TDefaultArrayAllocator& ) = delete;
    TDefaultArrayAllocator& operator=( const TDefaultArrayAllocator& ) = delete;

    FORCEINLINE TDefaultArrayAllocator() noexcept
        : Allocation( nullptr )
    {
    }

    FORCEINLINE TDefaultArrayAllocator( TDefaultArrayAllocator&& Other ) noexcept
        : Allocation( Other.Allocation )
    {
        Other.Allocation = nullptr;
    }

    FORCEINLINE ~TDefaultArrayAllocator()
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

    FORCEINLINE void MoveFrom( TDefaultArrayAllocator&& Other )
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

    FORCEINLINE TDefaultArrayAllocator& operator=( TDefaultArrayAllocator&& Other )
    {
        MoveFrom( Forward<TDefaultArrayAllocator>( Other ) );
        return *this;
    }

private:
    ElementType* Allocation;
};

/* InlineAllocator allocator that has a small fixed size memory, then allocates from malloc */
template<typename T, const int32 InlineBytes = 32>
class TInlineArrayAllocator
{
public:
    using ElementType = T;
    using SizeType = int32;

    static_assert(InlineBytes > sizeof( void* ), "InlineBytes has to be larger that the size of a void*");

    /* Since we do not store the size of the allocation we cannot copy. TODO: See if this is a better approcach */
    TInlineArrayAllocator( const TInlineArrayAllocator& ) = delete;
    TInlineArrayAllocator& operator=( const TInlineArrayAllocator& ) = delete;

    /* Default constructor */
    FORCEINLINE TInlineArrayAllocator() noexcept
        : Size( 0 )
        , Allocation( nullptr )
    {
    }

    /* Move constructor */
    FORCEINLINE TInlineArrayAllocator( TInlineArrayAllocator&& Other ) noexcept
        : Size( Other.Size )
        , Allocation( nullptr )
    {
        Memory::Memexchange( InlineAllocation, Other.InlineAllocation, InlineBytes );
        Other.Size = 0;
    }

    FORCEINLINE ~TInlineArrayAllocator()
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
    FORCEINLINE void MoveFrom( TInlineArrayAllocator&& Other )
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
    FORCEINLINE TInlineArrayAllocator& operator=( TInlineArrayAllocator&& Other )
    {
        MoveFrom( Forward<TInlineArrayAllocator>( Other ) );
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