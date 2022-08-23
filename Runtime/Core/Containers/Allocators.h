#pragma once
#include "Core/CoreTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/AlignedStorage.h"

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(push)
	#pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TArrayAllocatorInterface 

template<typename T>
class TArrayAllocatorInterface
{
public:
    using ElementType = T;
    using SizeType    = int32;

    TArrayAllocatorInterface() noexcept = default;
    ~TArrayAllocatorInterface()         = default;

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
    NODISCARD FORCEINLINE ElementType* GetAllocation() noexcept { return nullptr; }

    /**
     * @brief: Retrieve the allocation
     *
     * @return: Returns the allocation
     */
    NODISCARD FORCEINLINE const ElementType* GetAllocation() const noexcept { return nullptr; }

    /**
     * @brief: Returns the current state of the allocation
     *
     * @return: Returns true or false if there is an allocation
     */
    NODISCARD FORCEINLINE bool HasAllocation() const noexcept { return false; }

    /**
     * @brief: Returns the current state of the allocation
     *
     * @return: Returns true or false if the allocation is allocated on the heap
     */
    NODISCARD FORCEINLINE bool IsHeapAllocated() const noexcept { return false; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TDefaultArrayAllocator - Default allocator that allocates from malloc

template<typename T>
class TDefaultArrayAllocator
{
public:
    using ElementType = T;
    using SizeType    = int32;

    FORCEINLINE TDefaultArrayAllocator() noexcept
        : Allocation(nullptr)
    { }

    FORCEINLINE ~TDefaultArrayAllocator()
    {
        Free();
    }

    FORCEINLINE ElementType* Realloc(SizeType CurrentCount, SizeType NewCount) noexcept
    {
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

        Allocation       = Other.Allocation;
        Other.Allocation = nullptr;
    }

    NODISCARD FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return Allocation;
    }

    NODISCARD FORCEINLINE const ElementType* GetAllocation() const noexcept
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TInlineArrayAllocator

template<
    typename T,
    int32 NumInlineElements>
class TInlineArrayAllocator
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TInlineAllocation - Wrapper class for inline allocated bytes

    template<
        typename ElementType,
        int32 NumElements>
    class TInlineStorage
    {
    public:
        using SizeType = int32;

        TInlineStorage()  = default;
        ~TInlineStorage() = default;

        NODISCARD FORCEINLINE ElementType* GetElements() noexcept
        {
            return reinterpret_cast<ElementType*>(InlineAllocation);
        }

        NODISCARD FORCEINLINE const ElementType* GetElements() const noexcept
        {
            return reinterpret_cast<const ElementType*>(InlineAllocation);
        }

    public:
        NODISCARD CONSTEXPR SizeType GetSize() const noexcept
        {
            return sizeof(InlineAllocation);
        }

    private:
        TAlignedStorage<sizeof(ElementType), AlignmentOf<ElementType>> InlineAllocation[NumElements];
    };

public:
    using ElementType = T;
    using SizeType    = int32;

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
        DynamicAllocation.Free();
    }

    FORCEINLINE void MoveFrom(TInlineArrayAllocator&& Other)
    {
        Check(this != &Other);

        if (!Other.DynamicAllocation.HasAllocation())
        {
            ::RelocateElements<ElementType>(InlineAllocation.GetElements(), Other.InlineAllocation.GetElements(), NumInlineElements);
        }

        // This call Free's any potential dynamic allocation we own
        DynamicAllocation.MoveFrom(Move(Other.DynamicAllocation));
    }

    NODISCARD FORCEINLINE ElementType* GetAllocation() noexcept
    {
        return IsHeapAllocated() ? DynamicAllocation.GetAllocation() : InlineAllocation.GetElements();
    }

    NODISCARD FORCEINLINE const ElementType* GetAllocation() const noexcept
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

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
	#pragma clang diagnostic pop
#endif
