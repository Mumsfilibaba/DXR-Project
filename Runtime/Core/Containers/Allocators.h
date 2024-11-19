#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

template<typename ElementType>
struct TArrayAllocatorInterface
{
    typedef int32 SIZETYPE;

    /**
     * @brief              - Reallocates the allocation
     * @param CurrentCount - Current number of elements that are allocated 
     * @param NewCount     - The new number of elements to allocate
     */
    FORCEINLINE ElementType* Realloc(SIZETYPE CurrentCount, SIZETYPE NewCount) { return nullptr; }

    /**
     * @brief - Free the allocation
     */
    FORCEINLINE void Free() { }

    /**
     * @brief       - Move allocation from another allocator-instance
     * @param Other - Other allocator instance
     */
    FORCEINLINE void MoveFrom(TArrayAllocatorInterface&& Other) { }

    /**
     * @brief  - Retrieve the allocation
     * @return - Returns the allocation
     */
    NODISCARD FORCEINLINE ElementType* GetAllocation() const { return nullptr; }

    /**
     * @brief  - Returns the current state of the allocation
     * @return - Returns true or false if there is an allocation
     */
    NODISCARD FORCEINLINE bool HasAllocation() const { return false; }

    /**
     * @brief  - Returns the current state of the allocation
     * @return - Returns true or false if the allocation is allocated on the heap
     */
    NODISCARD FORCEINLINE bool IsHeapAllocated() const { return false; }
};


template<typename ElementType>
class TDefaultArrayAllocator
{
public:
    typedef int32 SIZETYPE;

    TDefaultArrayAllocator() = default;

    FORCEINLINE ElementType* Realloc(SIZETYPE CurrentCount, SIZETYPE NewCount)
    {
        Allocation = reinterpret_cast<ElementType*>(FMemory::Realloc(Allocation, NewCount * sizeof(ElementType)));
        return Allocation;
    }

    FORCEINLINE void Free()
    {
        if (Allocation)
        {
            FMemory::Free(Allocation);
            Allocation = nullptr;
        }
    }

    FORCEINLINE void MoveFrom(TDefaultArrayAllocator&& Other)
    {
        CHECK(this != &Other);
        Free();
        Allocation = Other.Allocation;
        Other.Allocation = nullptr;
    }

    NODISCARD FORCEINLINE ElementType* GetAllocation() const
    {
        return Allocation;
    }

    NODISCARD FORCEINLINE bool HasAllocation() const
    {
        return Allocation != nullptr;
    }

    NODISCARD FORCEINLINE bool IsHeapAllocated() const
    {
        return true;
    }

private:
    ElementType* Allocation = nullptr;
};


template<typename ElementType, int32 NumInlineElements>
class TInlineArrayAllocator
{
    template<int32 NumElements>
    class FInlineStorage
    {
    public:
        typedef int32 SIZETYPE;

        NODISCARD constexpr ElementType* GetElements() const
        {
            return reinterpret_cast<ElementType*>(InlineAllocation);
        }

        NODISCARD constexpr SIZETYPE Size() const
        {
            return sizeof(InlineAllocation);
        }

    private:
        mutable TAlignedBytes<sizeof(ElementType), TAlignmentOf<ElementType>::Value> InlineAllocation[NumElements];
    };

public:
    typedef int32 SIZETYPE;

    TInlineArrayAllocator()
    {
        FMemory::Memzero(InlineAllocation.GetElements(), InlineAllocation.Size());
    }

    FORCEINLINE ~TInlineArrayAllocator()
    {
        Free();
    }

    FORCEINLINE ElementType* Realloc(SIZETYPE CurrentCount, SIZETYPE NewElementCount)
    {
        if (NewElementCount > NumInlineElements)
        {
            if (!DynamicAllocation.HasAllocation())
            {
                CHECK(CurrentCount <= NumInlineElements);
                DynamicAllocation.Realloc(CurrentCount, NewElementCount);
                ::RelocateObjects<ElementType>(reinterpret_cast<void*>(DynamicAllocation.GetAllocation()), InlineAllocation.GetElements(), CurrentCount);
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
                ::RelocateObjects<ElementType>(reinterpret_cast<void*>(InlineAllocation.GetElements()), DynamicAllocation.GetAllocation(), CurrentCount);
                Free();
            }

            return InlineAllocation.GetElements();
        }
    }

    FORCEINLINE void Free()
    {
        if (!DynamicAllocation.HasAllocation())
        {
            FMemory::Memzero(reinterpret_cast<void*>(InlineAllocation.GetElements()), InlineAllocation.Size());
        }
        else
        {
            DynamicAllocation.Free();
        }
    }

    FORCEINLINE void MoveFrom(TInlineArrayAllocator&& Other)
    {
        CHECK(this != &Other);

        if (!Other.DynamicAllocation.HasAllocation())
        {
            ::RelocateObjects<ElementType>(InlineAllocation.GetElements(), Other.InlineAllocation.GetElements(), NumInlineElements);
            FMemory::Memzero(reinterpret_cast<void*>(Other.InlineAllocation.GetElements()), Other.InlineAllocation.Size());
        }

        // This call Free's any potential dynamic allocation we own
        DynamicAllocation.MoveFrom(::Move(Other.DynamicAllocation));
    }

    NODISCARD FORCEINLINE ElementType* GetAllocation() const
    {
        return IsHeapAllocated() ? DynamicAllocation.GetAllocation() : InlineAllocation.GetElements();
    }

    NODISCARD FORCEINLINE bool HasAllocation() const
    {
        return DynamicAllocation.HasAllocation();
    }

    NODISCARD FORCEINLINE bool IsHeapAllocated() const
    {
        return DynamicAllocation.HasAllocation();
    }

private:
    FInlineStorage<NumInlineElements>   InlineAllocation;
    TDefaultArrayAllocator<ElementType> DynamicAllocation;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
