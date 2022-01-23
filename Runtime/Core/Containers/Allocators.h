#pragma once
#include "Core/CoreTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/ObjectHandling.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Allocator interface 

template<typename T>
class TArrayAllocatorInterface
{
public:
    using ElementType = T;
    using SizeType = int32;

    TArrayAllocatorInterface() noexcept = default;
    ~TArrayAllocatorInterface() = default;

    /**
     * Reallocates the allocation
     * 
     * @param CurrentCount: Current number of elements that are allocated 
     * @param NewCount: The new number of elements to allocate
     */
    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount) noexcept { return nullptr; }

    /**
     * Free the allocation
     */
    FORCEINLINE void Free() noexcept { }

    /**
     * Move allocation from another allocator-instance
     * 
     * @param Other: Other allocator instance
     */
    FORCEINLINE void MoveFrom(TArrayAllocatorInterface&& Other) { }

    /**
     * Retrieve the allocation
     * 
     * @return: Returns the allocation
     */
    FORCEINLINE ElementType* GetAllocation() noexcept { return nullptr; }

    /**
     * Retrieve the allocation
     *
     * @return: Returns the allocation
     */
    FORCEINLINE const ElementType* GetAllocation() const noexcept { return nullptr; }

    /**
     * Returns the current state of the allocation
     *
     * @return: Returns true or false if there is an allocation
     */
    FORCEINLINE bool HasAllocation() const noexcept { return false; }

    /**
     * Returns the current state of the allocation
     *
     * @return: Returns true or false if the allocation is allocated on the heap
     */
    FORCEINLINE bool IsHeapAllocated() const noexcept { return false; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Default allocator that allocates from malloc

template<typename T>
class TDefaultArrayAllocator
{
public:

    using ElementType = T;
    using SizeType = int32;

    FORCEINLINE TDefaultArrayAllocator() noexcept
        : Allocation(nullptr)
    {
    }

    FORCEINLINE ~TDefaultArrayAllocator()
    {
        Free();
    }

    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount) noexcept
    {
        UNREFERENCED_VARIABLE(CurrentCount);

        Allocation = CMemory::Realloc<ElementType>(Allocation, NewCount);
        return Allocation;
    }

    FORCEINLINE void Free() noexcept
    {
        if (Allocation)
        {
            CMemory::Free(Allocation);
            Allocation = nullptr;
        }
    }

    FORCEINLINE void MoveFrom(TDefaultArrayAllocator&& Other)
    {
        Assert(this != &Other);

        Free();

        Allocation = Other.Allocation;
        Other.Allocation = nullptr;
    }

    FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return Allocation;
    }

    FORCEINLINE const ElementType* GetAllocation() const noexcept
    {
        return Allocation;
    }

    FORCEINLINE bool HasAllocation() const noexcept
    {
        return (Allocation != nullptr);
    }

    FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return true;
    }

private:
    ElementType* Allocation;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Wrapper class for inline allocated bytes

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// InlineAllocator allocator that has a small fixed size memory, then allocates from TDefaultArrayAllocator

template<typename T, int32 NumInlineElements>
class TInlineArrayAllocator
{
public:

    using ElementType = T;
    using SizeType = int32;

    FORCEINLINE TInlineArrayAllocator() noexcept
        : InlineAllocation()
        , DynamicAllocator()
    {
    }

    FORCEINLINE ~TInlineArrayAllocator()
    {
        Free();
    }

    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewElementCount) noexcept
    {
        if (NewElementCount > NumInlineElements)
        {
            if (!DynamicAllocator.HasAllocation())
            {
                Assert(CurrentCount <= NumInlineElements);

                DynamicAllocator.Realloc(CurrentCount, NewElementCount);
                RelocateRange<ElementType>(reinterpret_cast<void*>(DynamicAllocator.GetAllocation()), InlineAllocation.GetElements(), CurrentCount);
            }
            else
            {
                DynamicAllocator.Realloc(CurrentCount, NewElementCount);
            }

            return DynamicAllocator.GetAllocation();
        }
        else
        {
            if (DynamicAllocator.HasAllocation())
            {
                CurrentCount = (CurrentCount <= NumInlineElements) ? CurrentCount : NumInlineElements;

                RelocateRange<ElementType>(reinterpret_cast<void*>(InlineAllocation.GetElements()), DynamicAllocator.GetAllocation(), CurrentCount);
                Free();
            }

            return InlineAllocation.GetElements();
        }
    }

    FORCEINLINE void Free() noexcept
    {
        DynamicAllocator.Free();
    }

    FORCEINLINE void MoveFrom(TInlineArrayAllocator&& Other)
    {
        Assert(this != &Other);

        if (!Other.DynamicAllocator.HasAllocation())
        {
            RelocateRange<ElementType>(InlineAllocation.GetElements(), Other.InlineAllocation.GetElements(), NumInlineElements);
        }

        DynamicAllocator.MoveFrom(Move(Other.DynamicAllocator));
    }

    FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return IsHeapAllocated() ? DynamicAllocator.GetAllocation() : InlineAllocation.GetElements();
    }

    FORCEINLINE const ElementType* GetAllocation() const noexcept
    {
        return IsHeapAllocated() ? DynamicAllocator.GetAllocation() : InlineAllocation.GetElements();
    }

    FORCEINLINE bool HasAllocation() const noexcept
    {
        return DynamicAllocator.HasAllocation();
    }

    FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return DynamicAllocator.HasAllocation();
    }

private:

    // TODO: Pack memory more efficiently?

    /* Inline bytes */
    TInlineAllocation<ElementType, NumInlineElements> InlineAllocation;
    /* Dynamic allocation */
    TDefaultArrayAllocator<ElementType> DynamicAllocator;
};
