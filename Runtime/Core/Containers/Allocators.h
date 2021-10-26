#pragma once
#include "CoreTypes.h"

#include "Core/Memory/Memory.h"
#include "Core/Templates/ObjectHandling.h"

/* Default allocator that allocates from malloc */
template<typename T>
class TDefaultArrayAllocator
{
public:
    using ElementType = T;
    using SizeType = int32;

    /* Default constructor */
    FORCEINLINE TDefaultArrayAllocator() noexcept
        : Allocation( nullptr )
    {
    }

    /* Release storage */
    FORCEINLINE ~TDefaultArrayAllocator()
    {
        Free();
    }

    /* Allocates memory if needed, uses Memory::Realloc */
    FORCEINLINE ElementType* Realloc( SizeType CurrentCount, SizeType NewCount ) noexcept
    {
        UNREFERENCED_VARIABLE( CurrentCount );

        Allocation = CMemory::Realloc<ElementType>( Allocation, NewCount );
        return Allocation;
    }

    /* Frees this allocator's memory if necessary */
    FORCEINLINE void Free() noexcept
    {
        if ( Allocation )
        {
            CMemory::Free( Allocation );
            Allocation = nullptr;
        }
    }

    /* Populates this allocator from another */
    FORCEINLINE void MoveFrom( TDefaultArrayAllocator&& Other )
    {
        Assert( this != &Other );

        Free();

        Allocation = Other.Allocation;
        Other.Allocation = nullptr;
    }

    /* Retrieve the allocation */
    FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return Allocation;
    }

    /* Retrieve the allocation */
    FORCEINLINE const ElementType* GetAllocation() const noexcept
    {
        return Allocation;
    }

    /* Check if the allocator contains an allocation */
    FORCEINLINE bool HasAllocation() const noexcept
    {
        return (Allocation != nullptr);
    }

    /* Checks weather there is a heap allocation or not */
    FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return true;
    }

    /* Calculates size for a certain number of elements */
    static FORCEINLINE SizeType CalculateSize( SizeType NumElements ) noexcept
    {
        return sizeof( ElementType ) * NumElements;
    }

private:
    ElementType* Allocation;
};

/* Wrapper class for inline allocated bytes */
template<typename InlineType, int32 NumElements>
class TInlineAllocation
{
public:

    using SizeType = int32;

    TInlineAllocation() = default;
    ~TInlineAllocation() = default;

    FORCEINLINE InlineType* GetElements() noexcept
    {
        return reinterpret_cast<InlineType*>(InlineAllocation);
    }

    FORCEINLINE const InlineType* GetElements() const noexcept
    {
        return reinterpret_cast<const InlineType*>(InlineAllocation);
    }

public:
    constexpr SizeType GetSize() const noexcept
    {
        return InlineBytes;
    }

private:

    enum
    {
        InlineBytes = NumElements * sizeof( InlineType )
    };

    int8 InlineAllocation[InlineBytes];
};

/* InlineAllocator allocator that has a small fixed size memory, then allocates from TDefaultArrayAllocator */
template<typename T, int32 NumInlineElements>
class TInlineArrayAllocator
{
public:

    using ElementType = T;
    using SizeType = int32;

    /* Default constructor */
    FORCEINLINE TInlineArrayAllocator() noexcept
        : InlineAllocation()
        , DynamicAllocator()
    {
    }

    /* Destructor frees the memory */
    FORCEINLINE ~TInlineArrayAllocator()
    {
        Free();
    }

    /* Allocates memory if needed */
    FORCEINLINE ElementType* Realloc( SizeType CurrentCount, SizeType NewElementCount ) noexcept
    {
        // If allocation is larger than inline-storage, allocate on the heap
        if ( NewElementCount > NumInlineElements )
        {
            // If we did not have a allocation then the inline storage was used, copy it into dynamic memory
            if ( !DynamicAllocator.HasAllocation() )
            {
                Assert( CurrentCount <= NumInlineElements );

                DynamicAllocator.Realloc( CurrentCount, NewElementCount );
                RelocateRange<ElementType>( reinterpret_cast<void*>(DynamicAllocator.GetAllocation()), InlineAllocation.GetElements(), CurrentCount );
            }
            else
            {
                DynamicAllocator.Realloc( CurrentCount, NewElementCount );
            }

            return DynamicAllocator.GetAllocation();
        }
        else
        {
            // Copy the old allocation over to the inline allocation
            if ( DynamicAllocator.HasAllocation() )
            {
                // Only copy as much as can fit into the inline allocation
                CurrentCount = (CurrentCount <= NumInlineElements) ? CurrentCount : NumInlineElements;

                RelocateRange<ElementType>( reinterpret_cast<void*>(InlineAllocation.GetElements()), DynamicAllocator.GetAllocation(), CurrentCount );
                Free();
            }

            return InlineAllocation.GetElements();
        }
    }

    /* Make sure that the allocation is freed */
    FORCEINLINE void Free() noexcept
    {
        DynamicAllocator.Free();
    }

    /* Move from one allocator to another */
    FORCEINLINE void MoveFrom( TInlineArrayAllocator&& Other )
    {
        Assert( this != &Other );

        // Relocate static storage if necessary
        if ( !Other.DynamicAllocator.HasAllocation() )
        {
            RelocateRange<ElementType>( InlineAllocation.GetElements(), Other.InlineAllocation.GetElements(), NumInlineElements );
        }

        DynamicAllocator.MoveFrom( Move( Other.DynamicAllocator ) );
    }

    /* Checks weather there is a valid allocation or not */
    FORCEINLINE bool HasAllocation() const noexcept
    {
        return DynamicAllocator.HasAllocation();
    }

    /* Checks weather there is a heap allocation or not */
    FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return DynamicAllocator.HasAllocation();
    }

    /* Calculates size for a certain number of elements */
    static FORCEINLINE SizeType CalculateSize( SizeType NumElements ) noexcept
    {
        return sizeof( ElementType ) * NumElements;
    }

    /* Retrieve the allocation */
    FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return IsHeapAllocated() ? DynamicAllocator.GetAllocation() : InlineAllocation.GetElements();
    }

    /* Retrieve the allocation */
    FORCEINLINE const ElementType* GetAllocation() const noexcept
    {
        return IsHeapAllocated() ? DynamicAllocator.GetAllocation() : InlineAllocation.GetElements();
    }

private:

    // TODO: Pack memory more efficiently?

    /* Inline bytes */
    TInlineAllocation<ElementType, NumInlineElements> InlineAllocation;

    /* Dynamic allocation */
    TDefaultArrayAllocator<ElementType> DynamicAllocator;
};
