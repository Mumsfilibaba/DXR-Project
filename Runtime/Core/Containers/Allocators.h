#pragma once
#include "Core/CoreTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/AlignedStorage.h"

#if defined(COMPILER_MSVC)
	#pragma warning(push)
	#pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Allocator interface 

template<typename T>
class TArrayAllocatorInterface
{
public:
    using ElementType = T;
    using SizeType    = int32;

    TArrayAllocatorInterface() noexcept = default;
    ~TArrayAllocatorInterface() = default;

    /**
     * @brief: Reallocates the allocation
     * 
     * @param CurrentCount: Current number of elements that are allocated 
     * @param NewCount: The new number of elements to allocate
     */
    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount) noexcept { return nullptr; }

    /**
     * @brief: Free the allocation
     */
    FORCEINLINE void Free() noexcept { }

    /**
     * @brief: Move allocation from another allocator-instance
     * 
     * @param Other: Other allocator instance
     */
    FORCEINLINE void MoveFrom(TArrayAllocatorInterface&& Other) { }

    /**
     * @brief: Retrieve the allocation
     * 
     * @return: Returns the allocation
     */
    FORCEINLINE ElementType* GetAllocation() noexcept { return nullptr; }

    /**
     * @brief: Retrieve the allocation
     *
     * @return: Returns the allocation
     */
    FORCEINLINE const ElementType* GetAllocation() const noexcept { return nullptr; }

    /**
     * @brief: Returns the current state of the allocation
     *
     * @return: Returns true or false if there is an allocation
     */
    FORCEINLINE bool HasAllocation() const noexcept { return false; }

    /**
     * @brief: Returns the current state of the allocation
     *
     * @return: Returns true or false if the allocation is allocated on the heap
     */
    FORCEINLINE bool IsHeapAllocated() const noexcept { return false; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TDefaultArrayAllocator - Default allocator that allocates from malloc

template<typename T>
class TDefaultArrayAllocator
{
public:

    using ElementType = T;
    using SizeType = int32;

    FORCEINLINE TDefaultArrayAllocator() noexcept
        : Allocation(nullptr)
    { }

    FORCEINLINE ~TDefaultArrayAllocator()
    {
        Free();
    }

    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount) noexcept
    {
        UNREFERENCED_VARIABLE(CurrentCount);

        Allocation = FMemory::Realloc<ElementType>(Allocation, NewCount);
        return Allocation;
    }

    FORCEINLINE void Free() noexcept
    {
        if (Allocation)
        {
            FMemory::Free(Allocation);
            Allocation = nullptr;
        }
    }

    FORCEINLINE void MoveFrom(TDefaultArrayAllocator&& Other)
    {
        Check(this != &Other);

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
// TInlineAllocation - Wrapper class for inline allocated bytes

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
    CONSTEXPR SizeType GetSize() const noexcept
    {
        return sizeof(InlineAllocation);
    }

private:
    TAlignedStorage<sizeof(InlineType), AlignmentOf<InlineType>> InlineAllocation[NumElements];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TInlineArrayAllocator - InlineAllocator allocator that has a small fixed size memory, then allocates from TDefaultArrayAllocator

template<typename T, int32 NumInlineElements>
class TInlineArrayAllocator
{
public:

    using ElementType = T;
    using SizeType = int32;

    FORCEINLINE TInlineArrayAllocator() noexcept
        : InlineAllocation()
        , DynamicAllocation()
    { }

    FORCEINLINE ~TInlineArrayAllocator()
    {
        Free();
    }

    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewElementCount) noexcept
    {
        if (NewElementCount > NumInlineElements)
        {
            if (!DynamicAllocation.HasAllocation())
            {
                Check(CurrentCount <= NumInlineElements);

                DynamicAllocation.Realloc(CurrentCount, NewElementCount);
                RelocateRange<ElementType>(reinterpret_cast<void*>(DynamicAllocation.GetAllocation()), InlineAllocation.GetElements(), CurrentCount);
            }
            else
            {
                DynamicAllocation.Realloc(CurrentCount, NewElementCount);
            }

            return DynamicAllocation.GetAllocation();
        }
        else
        {
            if (DynamicAllocation.HasAllocation())
            {
                CurrentCount = (CurrentCount <= NumInlineElements) ? CurrentCount : NumInlineElements;

                RelocateRange<ElementType>(reinterpret_cast<void*>(InlineAllocation.GetElements()), DynamicAllocation.GetAllocation(), CurrentCount);
                Free();
            }

            return InlineAllocation.GetElements();
        }
    }

    FORCEINLINE void Free() noexcept
    {
        DynamicAllocation.Free();
    }

    FORCEINLINE void MoveFrom(TInlineArrayAllocator&& Other)
    {
        Check(this != &Other);

        if (!Other.DynamicAllocation.HasAllocation())
        {
            RelocateRange<ElementType>(InlineAllocation.GetElements(), Other.InlineAllocation.GetElements(), NumInlineElements);
        }

        DynamicAllocation.MoveFrom(Move(Other.DynamicAllocation));
    }

    FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return IsHeapAllocated() ? DynamicAllocation.GetAllocation() : InlineAllocation.GetElements();
    }

    FORCEINLINE const ElementType* GetAllocation() const noexcept
    {
        return IsHeapAllocated() ? DynamicAllocation.GetAllocation() : InlineAllocation.GetElements();
    }

    FORCEINLINE bool HasAllocation() const noexcept
    {
        return DynamicAllocation.HasAllocation();
    }

    FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return DynamicAllocation.HasAllocation();
    }

private:
    TInlineAllocation<ElementType, NumInlineElements> InlineAllocation;
    TDefaultArrayAllocator<ElementType>               DynamicAllocation;
};

#if defined(COMPILER_MSVC)
	#pragma warning(pop)
#elif defined(COMPILER_CLANG)
	#pragma clang diagnostic pop
#endif
