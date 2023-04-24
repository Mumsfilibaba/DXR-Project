#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

template<typename ElementType>
class TArrayAllocatorInterface
{
public:
    using SizeType = int32;

    TArrayAllocatorInterface() noexcept = default;
    ~TArrayAllocatorInterface()         = default;

    /**
     * @brief              - Reallocates the allocation
     * @param CurrentCount - Current number of elements that are allocated 
     * @param NewCount     - The new number of elements to allocate
     */
    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount) noexcept { return nullptr; }

    /**
     * @brief - Free the allocation
     */
    FORCEINLINE void Free() noexcept { }

    /**
     * @brief       - Move allocation from another allocator-instance
     * @param Other - Other allocator instance
     */
    FORCEINLINE void MoveFrom(TArrayAllocatorInterface&& Other) { }

    /**
     * @brief  - Retrieve the allocation
     * @return - Returns the allocation
     */
    NODISCARD FORCEINLINE ElementType* GetAllocation() const noexcept { return nullptr; }

    /**
     * @brief  - Returns the current state of the allocation
     * @return - Returns true or false if there is an allocation
     */
    NODISCARD FORCEINLINE bool HasAllocation() const noexcept { return false; }

    /**
     * @brief  - Returns the current state of the allocation
     * @return - Returns true or false if the allocation is allocated on the heap
     */
    NODISCARD FORCEINLINE bool IsHeapAllocated() const noexcept { return false; }
};


template<typename ElementType>
class TDefaultArrayAllocator
{
public:
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
        Allocation = reinterpret_cast<ElementType*>(FMemory::Realloc(Allocation, NewCount * sizeof(ElementType)));
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
        CHECK(this != &Other);
        Free();
        Allocation = Other.Allocation;
        Other.Allocation = nullptr;
    }

    NODISCARD FORCEINLINE ElementType* GetAllocation() const noexcept
    {
        return Allocation;
    }

    NODISCARD FORCEINLINE bool HasAllocation() const noexcept
    {
        return (Allocation != nullptr);
    }

    NODISCARD FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return true;
    }

private:
    ElementType* Allocation;
};


template<typename ElementType, int32 NumInlineElements>
class TInlineArrayAllocator
{
    template<typename ElementType, int32 NumElements>
    class TInlineStorage
    {
    public:
        using SizeType = int32;

        NODISCARD constexpr ElementType* GetElements() const noexcept
        {
            return reinterpret_cast<ElementType*>(InlineAllocation);
        }

        NODISCARD constexpr SizeType Size() const noexcept
        {
            return sizeof(InlineAllocation);
        }

    private:
        mutable TAlignedBytes<sizeof(ElementType), AlignmentOf<ElementType>> InlineAllocation[NumElements];
    };

public:
    using SizeType = int32;

    FORCEINLINE TInlineArrayAllocator() noexcept
        : InlineAllocation()
        , DynamicAllocation()
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
            if (!DynamicAllocation.HasAllocation())
            {
                CHECK(CurrentCount <= NumInlineElements);
                DynamicAllocation.Realloc(CurrentCount, NewElementCount);
                ::RelocateElements<ElementType>(reinterpret_cast<void*>(DynamicAllocation.GetAllocation()), InlineAllocation.GetElements(), CurrentCount);
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
                ::RelocateElements<ElementType>(reinterpret_cast<void*>(InlineAllocation.GetElements()), DynamicAllocation.GetAllocation(), CurrentCount);
                Free();
            }

            return InlineAllocation.GetElements();
        }
    }

    FORCEINLINE void Free() noexcept
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
            ::RelocateElements<ElementType>(InlineAllocation.GetElements(), Other.InlineAllocation.GetElements(), NumInlineElements);
            FMemory::Memzero(reinterpret_cast<void*>(Other.InlineAllocation.GetElements()), Other.InlineAllocation.Size());
        }

        // This call Free's any potential dynamic allocation we own
        DynamicAllocation.MoveFrom(::Move(Other.DynamicAllocation));
    }

    NODISCARD FORCEINLINE ElementType* GetAllocation() const noexcept
    {
        return IsHeapAllocated() ? DynamicAllocation.GetAllocation() : InlineAllocation.GetElements();
    }

    NODISCARD FORCEINLINE bool HasAllocation() const noexcept
    {
        return DynamicAllocation.HasAllocation();
    }

    NODISCARD FORCEINLINE bool IsHeapAllocated() const noexcept
    {
        return DynamicAllocation.HasAllocation();
    }

private:
    TInlineStorage<ElementType, NumInlineElements> InlineAllocation;
    TDefaultArrayAllocator<ElementType>            DynamicAllocation;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
