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

        NODISCARD constexpr ElementType* GetAllocation() const
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
        Memory::Memzero(InlineAllocation.GetAllocation(), InlineAllocation.Size());
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
                    ::RelocateObjects<ElementType>(reinterpret_cast<void*>(DynamicAllocation.GetAllocation()), InlineAllocation.GetAllocation(), CurrentCount);
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
                    ::RelocateObjects<ElementType>(reinterpret_cast<void*>(InlineAllocation.GetAllocation()), DynamicAllocation.GetAllocation(), CurrentCount);
                }

                Free();
            }

            return InlineAllocation.GetAllocation();
        }
    }

    FORCEINLINE void Free()
    {
        if (!DynamicAllocation.HasAllocation())
        {
            Memory::Memzero(reinterpret_cast<void*>(InlineAllocation.GetAllocation()), InlineAllocation.Size());
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
                ::RelocateObjects<ElementType>(InlineAllocation.GetAllocation(), Other.InlineAllocation.GetAllocation(), NumElements);
            }

            // Clear the source inline storage after relocation (raw storage, safe).
            Memory::Memzero(reinterpret_cast<void*>(Other.InlineAllocation.GetAllocation()), Other.InlineAllocation.Size());
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
            Memory::Memmove(reinterpret_cast<void*>(InlineAllocation.GetAllocation()), reinterpret_cast<const void*>(Other.InlineAllocation.GetAllocation()), InlineAllocation.Size());
            Memory::Memzero(reinterpret_cast<void*>(Other.InlineAllocation.GetAllocation()), Other.InlineAllocation.Size());
        }

        DynamicAllocation.MoveFrom(::Move(Other.DynamicAllocation));
    }

    NODISCARD FORCEINLINE ElementType* GetAllocation() const
    {
        return IsHeapAllocated() ? DynamicAllocation.GetAllocation() : InlineAllocation.GetAllocation();
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

template<typename ElementType>
struct THashTableAllocatorInterface
{
    typedef int32 SizeType;

    /**
     * @brief Allocate storage for the specified number of slots, replacing any current allocation
     * @param SlotCount Number of slots to allocate, which is always a power of two
     */
    FORCEINLINE void Allocate(SizeType SlotCount) { }

    /**
     * @brief Free the allocation
     */
    FORCEINLINE void Free() { }

    /**
     * @brief Move the allocation from another allocator-instance
     * @param Other Other allocator instance
     * @param SlotCount Number of slots stored in the allocation of the other instance
     */
    FORCEINLINE void MoveFrom(THashTableAllocatorInterface&& Other, SizeType SlotCount) { }

    /**
     * @brief Retrieve the slot array, which stores the elements of the table
     * @param SlotCount Number of slots the storage was allocated for
     * @return Returns the slot array, or nullptr if there is no allocation
     */
    NODISCARD FORCEINLINE ElementType* GetSlots(SizeType SlotCount) const { return nullptr; }

    /**
     * @brief Retrieve the control array, which stores one metadata byte per slot
     * @param SlotCount Number of slots the storage was allocated for
     * @return Returns the control array, or nullptr if there is no allocation
     */
    NODISCARD FORCEINLINE uint8* GetControl(SizeType SlotCount) const { return nullptr; }

    /**
     * @brief Returns the current state of the allocation
     * @return Returns true or false if the allocation is allocated on the heap
     */
    NODISCARD FORCEINLINE bool IsHeapAllocated() const { return false; }
};

template<typename ElementType>
class TDefaultHashTableAllocator
{
public:
    typedef int32 SizeType;

    TDefaultHashTableAllocator() = default;

    FORCEINLINE void Allocate(SizeType SlotCount)
    {
        CHECK(SlotCount > 0);
        Free();

        const uint64 Bytes = static_cast<uint64>(SlotCount) * sizeof(ElementType) + static_cast<uint64>(SlotCount);
        Allocation = Memory::Malloc(Bytes);
        CHECK(Allocation != nullptr);
    }

    FORCEINLINE void Free()
    {
        if (Allocation)
        {
            Memory::Free(Allocation);
            Allocation = nullptr;
        }
    }

    FORCEINLINE void MoveFrom(TDefaultHashTableAllocator&& Other, SizeType)
    {
        CHECK(this != &Other);
        Free();
        Allocation       = Other.Allocation;
        Other.Allocation = nullptr;
    }

    NODISCARD FORCEINLINE ElementType* GetSlots(SizeType SlotCount) const
    {
        UNREFERENCED_VARIABLE(SlotCount);
        return static_cast<ElementType*>(Allocation);
    }

    NODISCARD FORCEINLINE uint8* GetControl(SizeType SlotCount) const
    {
        if (!Allocation)
        {
            return nullptr;
        }

        return reinterpret_cast<uint8*>(static_cast<ElementType*>(Allocation) + SlotCount);
    }

    NODISCARD FORCEINLINE bool IsHeapAllocated() const
    {
        return Allocation != nullptr;
    }

private:
    void* Allocation = nullptr;
};

template<typename ElementType, int32 NumInlineSlots>
class TInlineHashTableAllocator
{
    class TInlineStorage
    {
    public:
        NODISCARD constexpr ElementType* GetAllocation() const
        {
            return reinterpret_cast<ElementType*>(InlineAllocation);
        }

        NODISCARD constexpr uint64 Size() const
        {
            return sizeof(InlineAllocation);
        }

    private:
        mutable TAlignedBytes<sizeof(ElementType), TAlignmentOf<ElementType>::Value> InlineAllocation[NumInlineSlots];
    };

public:
    typedef int32 SizeType;

    TInlineHashTableAllocator()
    {
        Memory::Memzero(InlineSlots.GetAllocation(), InlineSlots.Size());
        Memory::Memzero(InlineControl, sizeof(InlineControl));
    }

    FORCEINLINE ~TInlineHashTableAllocator()
    {
        Free();
    }

    FORCEINLINE void Allocate(SizeType SlotCount)
    {
        CHECK(SlotCount > 0);

        if (SlotCount <= NumInlineSlots)
        {
            Heap.Free();
            return;
        }

        Heap.Allocate(SlotCount);
    }

    FORCEINLINE void Free()
    {
        if (Heap.IsHeapAllocated())
        {
            Heap.Free();
        }
        else
        {
            Memory::Memzero(InlineSlots.GetAllocation(), InlineSlots.Size());
            Memory::Memzero(InlineControl, sizeof(InlineControl));
        }
    }

    FORCEINLINE void MoveFrom(TInlineHashTableAllocator&& Other, SizeType SlotCount)
    {
        CHECK(this != &Other);

        Free();

        if (!Other.Heap.IsHeapAllocated())
        {
            if (SlotCount > 0)
            {
                CHECK(SlotCount <= NumInlineSlots);

                Memory::Memmove(InlineSlots.GetAllocation(), Other.InlineSlots.GetAllocation(), static_cast<uint64>(SlotCount) * sizeof(ElementType));
                Memory::Memmove(InlineControl, Other.InlineControl, static_cast<uint64>(SlotCount));
                Memory::Memzero(Other.InlineSlots.GetAllocation(), Other.InlineSlots.Size());
                Memory::Memzero(Other.InlineControl, sizeof(Other.InlineControl));
            }
        }

        Heap.MoveFrom(::Move(Other.Heap), SlotCount);
    }

    NODISCARD FORCEINLINE ElementType* GetSlots(SizeType SlotCount) const
    {
        return Heap.IsHeapAllocated() ? Heap.GetSlots(SlotCount) : InlineSlots.GetAllocation();
    }

    NODISCARD FORCEINLINE uint8* GetControl(SizeType SlotCount) const
    {
        return Heap.IsHeapAllocated() ? Heap.GetControl(SlotCount) : const_cast<uint8*>(InlineControl);
    }

    NODISCARD FORCEINLINE bool IsHeapAllocated() const
    {
        return Heap.IsHeapAllocated();
    }

private:
    TInlineStorage                          InlineSlots;
    uint8                                   InlineControl[NumInlineSlots];
    TDefaultHashTableAllocator<ElementType> Heap;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
