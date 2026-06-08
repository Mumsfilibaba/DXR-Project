#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

template<typename ElementType>
struct TArrayAllocatorInterface
{
    typedef int32 SizeType;

    /**
     * @brief Reallocates the allocation
     * @param CurrentCount Current number of elements that are allocated 
     * @param NewCount The new number of elements to allocate
     */
    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount) { return nullptr; }

    /**
     * @brief Free the allocation
     */
    FORCEINLINE void Free() { }

    /**
     * @brief Move allocation from another allocator-instance
     * @param Other Other allocator instance
     */
    FORCEINLINE void MoveFrom(TArrayAllocatorInterface&& Other) { }

    /**
     * @brief Move allocation from another allocator-instance (with element count)
     * @param Other Other allocator instance
     * @param NumElements Number of live elements stored in the allocation
     */
    FORCEINLINE void MoveFrom(TArrayAllocatorInterface&& Other, SizeType NumElements) { }

    /**
     * @brief Retrieve the allocation
     * @return Returns the allocation
     */
    NODISCARD FORCEINLINE ElementType* GetAllocation() const { return nullptr; }

    /**
     * @brief Returns the current state of the allocation
     * @return Returns true or false if there is an allocation
     */
    NODISCARD FORCEINLINE bool HasAllocation() const { return false; }

    /**
     * @brief Returns the current state of the allocation
     * @return Returns true or false if the allocation is allocated on the heap
     */
    NODISCARD FORCEINLINE bool IsHeapAllocated() const { return false; }
};


template<typename ElementType>
class TDefaultArrayAllocator
{
public:
    typedef int32 SizeType;

    TDefaultArrayAllocator() = default;

    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount)
    {
        UNREFERENCED_VARIABLE(CurrentCount);
        CHECK(NewCount >= 0);

        if (NewCount == 0)
        {
            Free();
            return nullptr;
        }

        const SizeType NewSizeInBytes = static_cast<SizeType>(NewCount) * sizeof(ElementType);
        CHECK((NewSizeInBytes / sizeof(ElementType)) == static_cast<SizeType>(NewCount));

        Allocation = reinterpret_cast<ElementType*>(Memory::Realloc(Allocation, NewSizeInBytes));
        return Allocation;
    }

    FORCEINLINE void Free()
    {
        if (Allocation)
        {
            Memory::Free(Allocation);
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

    FORCEINLINE void MoveFrom(TDefaultArrayAllocator&& Other, SizeType NumElements)
    {
        UNREFERENCED_VARIABLE(NumElements);
        MoveFrom(::Move(Other));
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
    class TInlineStorage
    {
    public:
        typedef int32 SizeType;

        NODISCARD constexpr ElementType* GetElements() const
        {
            return reinterpret_cast<ElementType*>(InlineAllocation);
        }

        NODISCARD constexpr SizeType Size() const
        {
            return sizeof(InlineAllocation);
        }

    private:
        mutable TAlignedBytes<sizeof(ElementType), TAlignmentOf<ElementType>::Value> InlineAllocation[NumElements];
    };

public:
    typedef int32 SizeType;

    TInlineArrayAllocator()
    {
        Memory::Memzero(InlineAllocation.GetElements(), InlineAllocation.Size());
    }

    FORCEINLINE ~TInlineArrayAllocator()
    {
        Free();
    }

    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewElementCount)
    {
        CHECK(CurrentCount >= 0);
        CHECK(NewElementCount >= 0);

        if (NewElementCount > NumInlineElements)
        {
            if (!DynamicAllocation.HasAllocation())
            {
                CHECK(CurrentCount <= NumInlineElements);
                DynamicAllocation.Realloc(CurrentCount, NewElementCount);

                if (CurrentCount > 0)
                {
                    ::RelocateObjects<ElementType>(reinterpret_cast<void*>(DynamicAllocation.GetAllocation()), InlineAllocation.GetElements(), CurrentCount);
                }
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
                if (CurrentCount > 0)
                {
                    ::RelocateObjects<ElementType>(reinterpret_cast<void*>(InlineAllocation.GetElements()), DynamicAllocation.GetAllocation(), CurrentCount);
                }

                Free();
            }

            return InlineAllocation.GetElements();
        }
    }

    FORCEINLINE void Free()
    {
        if (!DynamicAllocation.HasAllocation())
        {
            Memory::Memzero(reinterpret_cast<void*>(InlineAllocation.GetElements()), InlineAllocation.Size());
        }
        else
        {
            DynamicAllocation.Free();
        }
    }

    /**
     * @brief Move allocation from another allocator-instance (with element count)
     *
     * NOTE: The allocator does not know how many elements are live unless the container passes it in.
     * Moving inline storage without NumElements is undefined for non-reallocatable ElementType.
     */
    FORCEINLINE void MoveFrom(TInlineArrayAllocator&& Other, SizeType NumElements)
    {
        CHECK(this != &Other);
        CHECK(NumElements >= 0);

        // Move/relocate inline elements if the source is inline.
        if (!Other.DynamicAllocation.HasAllocation())
        {
            if (NumElements > 0)
            {
                CHECK(NumElements <= NumInlineElements);
                ::RelocateObjects<ElementType>(InlineAllocation.GetElements(), Other.InlineAllocation.GetElements(), NumElements);
            }

            // Clear the source inline storage after relocation (raw storage, safe).
            Memory::Memzero(reinterpret_cast<void*>(Other.InlineAllocation.GetElements()), Other.InlineAllocation.Size());
        }

        // Steal heap allocation (if any). This also frees our current heap allocation if we had one.
        DynamicAllocation.MoveFrom(::Move(Other.DynamicAllocation));
    }

    /**
     * @brief Move allocation from another allocator-instance
     *
     * Only safe for reallocatable element types, because we don't know how many elements are live.
     */
    FORCEINLINE void MoveFrom(TInlineArrayAllocator&& Other)
    {
        CHECK(this != &Other);

        static_assert(TIsReallocatable<ElementType>::Value,
            "TInlineArrayAllocator::MoveFrom(TInlineArrayAllocator&&) requires ElementType to be reallocatable. "
            "Use MoveFrom(TInlineArrayAllocator&&, SizeType NumElements) for non-reallocatable types.");

        if (!Other.DynamicAllocation.HasAllocation())
        {
            Memory::Memmove(reinterpret_cast<void*>(InlineAllocation.GetElements()), reinterpret_cast<const void*>(Other.InlineAllocation.GetElements()), InlineAllocation.Size());
            Memory::Memzero(reinterpret_cast<void*>(Other.InlineAllocation.GetElements()), Other.InlineAllocation.Size());
        }

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
    TInlineStorage<NumInlineElements>   InlineAllocation;
    TDefaultArrayAllocator<ElementType> DynamicAllocation;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
