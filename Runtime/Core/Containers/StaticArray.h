#pragma once
#include "Iterator.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/ArrayContainerHelper.h"

template<typename ElementType, int32 NUM_ELEMENTS>
struct TStaticArray
{
    using SizeType = int32;
    static_assert(TIsSigned<SizeType>::Value, "TStaticArray only supports a SizeType that's signed");
    static_assert(NUM_ELEMENTS > 0          , "TStaticArray does not support a zero element count");

    typedef TArrayIterator<TStaticArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TStaticArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticArray, const ElementType> ReverseConstIteratorType;

    enum : SizeType 
    {
        INVALID_INDEX = -1
    };

public:

    /**
     * @brief         - Checks that the pointer is a part of the array
     * @param Address - Address to check.
     * @return        - Returns true if the address belongs to the array
     */
    NODISCARD FORCEINLINE bool CheckAddress(const ElementType* Address) const noexcept
    {
        return (Address >= Elements) && (Address < (Elements + NUM_ELEMENTS));
    }

    /**
     * @brief  - Checks if an index is a valid index
     * @return - Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SizeType Index) const noexcept
    {
        return (Index >= 0) && (Index < NUM_ELEMENTS);
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE ElementType& FirstElement() noexcept
    {
        return Elements[0];
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        return Elements[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE ElementType& LastElement() noexcept
    {
        return Elements[NUM_ELEMENTS - 1];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const ElementType& LastElement() const noexcept
    {
        return Elements[NUM_ELEMENTS - 1];
    }

    /**
     * @brief              - Fill the container with the specified value
     * @param InputElement - Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ::AssignObjects(Elements, InputElement, NUM_ELEMENTS);
    }

    /**
     * @brief - Sets the array to zero
     */
    template<typename U = ElementType>
    FORCEINLINE typename TEnableIf<TIsTrivial<U>::Value>::Type Memzero()
    {
        FMemory::Memzero(Elements, SizeInBytes());
    }

    /**
     * @brief         - Returns the index of an element if it is present in the array, or -1 if it is not found
     * @param Element - Element to search for
     * @return        - The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start, *RESTRICT End = Start + NUM_ELEMENTS; Current != End; ++Current)
        {
            if (Element == *Current)
            {
                return static_cast<SizeType>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief           - Returns the index of the element that satisfy the conditions of a comparator
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SizeType FindWithPredicate(PredicateType&& Predicate) const noexcept
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start, *RESTRICT End = Start + NUM_ELEMENTS; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief         - Returns the index of an element if it is present in the array, or -1 if it is not found
     * @param Element - Element to search for
     * @return        - The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SizeType FindLast(const ElementType& Element) const noexcept
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start + NUM_ELEMENTS, *RESTRICT End = Start; Current != End;)
        {
            --Current;
            if (Element == *Current)
            {
                return static_cast<SizeType>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief           - Returns the index of the element that satisfy the conditions of a comparator
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SizeType FindLastWithPredicate(PredicateType&& Predicate) const noexcept
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start + NUM_ELEMENTS, *RESTRICT End = Start; Current != End;)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief         - Check if an element exists in the array
     * @param Element - Element to check for
     * @return        - Returns true if the element is found in the array and false if not
     */
    NODISCARD FORCEINLINE bool Contains(const ElementType& Element) const noexcept
    {
        return Find(Element) != INVALID_INDEX;
    }

    /**
     * @brief           - Check if an element that satisfies the conditions of a comparator exists in the array
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - Returns true if the comparator returned true for one element
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE bool ContainsWithPredicate(PredicateType&& Predicate) const noexcept
    {
        return FindWithPredicate(::Forward<PredicateType>(Predicate)) != INVALID_INDEX;
    }

    /**
     * @brief         - Perform some function on each element in the array
     * @param Functor - Callable that takes one element and perform some operation on it
     */
    template<class LambdaType>
    FORCEINLINE void Foreach(LambdaType&& Lambda)
    {
        for (const ElementType* RESTRICT Current = Elements, *RESTRICT End = Current + NUM_ELEMENTS; Current != End; ++Current)
        {
            Lambda(*Current);
        }
    }

    /**
     * @brief             - Swap two elements with each other
     * @param FirstIndex  - Index to the first element to Swap
     * @param SecondIndex - Index to the second element to Swap
     */
    FORCEINLINE void Swap(SizeType FirstIndex, SizeType SecondIndex) noexcept
    {
        CHECK(IsValidIndex(FirstIndex));
        CHECK(IsValidIndex(SecondIndex));
        ElementType* Array = Allocator.GetAllocation();
        ::Swap(Array[FirstIndex], Array[SecondIndex]);
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE ElementType* Data() noexcept
    {
        return Elements;
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const ElementType* Data() const noexcept
    {
        return Elements;
    }

public:

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SizeType Index) noexcept
    {
        CHECK(Index < NUM_ELEMENTS);
        return Elements[Index];
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        CHECK(Index < NUM_ELEMENTS);
        return Elements[Index];
    }

    /**
     * @brief       - Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param Other - Array to compare with
     * @return      - Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator==(const ArrayType& Other) const noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        return (ArraySize == FArrayContainerHelper::Size(Other)) ? ::CompareObjects<ElementType>(Allocator.GetAllocation(), FArrayContainerHelper::Data(Other), ArraySize) : (false);
    }

    /**
     * @brief       - Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param Other - Array to compare with
     * @return      - Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator!=(const ArrayType& Other) const noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        return !(*this == Other);
    }

public:

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns a the index to the last element of the array
     */
    NODISCARD constexpr SizeType LastElementIndex() const noexcept
    {
        return NUM_ELEMENTS - 1;
    }

    /**
     * @brief  - Returns the size of the container
     * @return - The current size of the container
     */
    NODISCARD constexpr SizeType Size() const noexcept
    {
        return NUM_ELEMENTS;
    }

    /**
     * @brief  - Returns the size of the container in bytes
     * @return - The current size of the container in bytes
     */
    NODISCARD constexpr SizeType SizeInBytes() const noexcept
    {
        return NUM_ELEMENTS * sizeof(ElementType);
    }

    /**
     * @brief  - Returns the capacity of the container
     * @return - The current capacity of the container
     */
    NODISCARD constexpr SizeType Capacity() const noexcept
    {
        return NUM_ELEMENTS;
    }

    /**
     * @brief  - Returns the capacity of the container in bytes
     * @return - The current capacity of the container in bytes
     */
    NODISCARD constexpr SizeType CapacityInBytes() const noexcept
    {
        return NUM_ELEMENTS * sizeof(ElementType);
    }

public: // Iterators

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType Iterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator() noexcept
    {
        return ReverseIteratorType(*this, NUM_ELEMENTS);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD  ReverseConstIteratorType ConstReverseIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, NUM_ELEMENTS);
    }

public: // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return ConstIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return IteratorType(*this, NUM_ELEMENTS); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return ConstIteratorType(*this, NUM_ELEMENTS); }

public:
    ElementType Elements[NUM_ELEMENTS];
};


template<typename ElementType, int32 NUM_ELEMENTS>
struct TIsTArrayType<TStaticArray<ElementType, NUM_ELEMENTS>>
{
    inline static constexpr bool Value = true;
};

template<typename ElementType, int32 NUM_ELEMENTS>
struct TIsContiguousContainer<TStaticArray<ElementType, NUM_ELEMENTS>>
{
    inline static constexpr bool Value = true;
};
