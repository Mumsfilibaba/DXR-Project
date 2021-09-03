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
    FORCEINLINE ElementType* Realloc( SizeType Count ) noexcept
    {
        Allocation = Memory::Realloc<ElementType>( Allocation, Count );
        return Allocation;
    }

    /* Frees this allocator's memory if necessary */
    FORCEINLINE void Free() noexcept
    {
        if ( Allocation )
        {
            Memory::Free( Allocation );
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

    /* Retrive the allocation */
    FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return Allocation;
    }

    /* Retrive the allocation */
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
        return sizeof(ElementType) * NumElements;
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
        InlineBytes = NumElements * sizeof(InlineType)
    };

    int8 InlineAllocation[InlineBytes];
};

/* InlineAllocator allocator that has a small fixed size memory, then allocates from TDefaultArrayAllocator */
template<typename T, int32 NumInlineElements>
class TInlineArrayAllocator
{
public:

    using ElementType = T;
    using SizeType    = int32;

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
    FORCEINLINE ElementType* Realloc( SizeType NewCount ) noexcept
    {
        // Allocate new
        if ( NewCount > NumInlineElements )
        {
            return DynamicAllocator.Realloc( NewCount );
        }
        else
        {
            Free();
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

        DynamicAllocator.MoveFrom( Other );
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
        return sizeof(ElementType) * NumElements;
    }

    /* Retrive the allocation */
    FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return IsHeapAllocated() ? DynamicAllocator.GetAllocation() : InlineAllocation.GetElements();
    }

    /* Retrive the allocation */
    FORCEINLINE const ElementType* GetAllocation() const noexcept
    {
        return IsHeapAllocated() ? DynamicAllocator.GetAllocation() : InlineAllocation.GetElements();
    }

private:

    // TODO: Pack memory more effienctly?

    /* Inline bytes */
    TInlineAllocation<ElementType, NumInlineElements> InlineAllocation;

    /* Dynamic allocation */
    TDefaultArrayAllocator<ElementType> DynamicAllocator;
};
