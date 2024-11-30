#pragma once
#include "Core/Containers/Iterator.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/ArrayContainerHelper.h"

template<typename ElementType, int32 NUM_ELEMENTS>
struct TStaticArray
{
    typedef int32 SIZETYPE;

    static_assert(TIsSigned<SIZETYPE>::Value, "TStaticArray only supports a SIZETYPE that's signed");
    static_assert(NUM_ELEMENTS > 0, "TStaticArray does not support a zero element count");

    typedef TArrayIterator<TStaticArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TStaticArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticArray, const ElementType> ReverseConstIteratorType;

    inline static constexpr SIZETYPE INVALID_INDEX = SIZETYPE(~0);

public:

    /**
     * @brief Checks that the pointer is a part of the array
     * @param Address Address to check.
     * @return Returns true if the address belongs to the array
     */
    NODISCARD FORCEINLINE bool CheckAddress(const ElementType* Address) const
    {
        return (Address >= Elements) && (Address < (Elements + NUM_ELEMENTS));
    }

    /**
     * @brief Checks if an index is a valid index
     * @return Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SIZETYPE Index) const
    {
        return (Index >= 0) && (Index < NUM_ELEMENTS);
    }

    /**
     * @brief Retrieve the first element of the array
     * @return Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE ElementType& FirstElement()
    {
        return Elements[0];
    }

    /**
     * @brief Retrieve the first element of the array
     * @return Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const ElementType& FirstElement() const
    {
        return Elements[0];
    }

    /**
     * @brief Retrieve the last element of the array
     * @return Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE ElementType& LastElement()
    {
        return Elements[NUM_ELEMENTS - 1];
    }

    /**
     * @brief Retrieve the last element of the array
     * @return Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const ElementType& LastElement() const
    {
        return Elements[NUM_ELEMENTS - 1];
    }

    /**
     * @brief Fill the container with the specified value
     * @param InputElement Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement)
    {
        ::AssignObjects(Elements, InputElement, NUM_ELEMENTS);
    }

    /**
     * @brief Sets the array to zero
     */
    template<typename U = ElementType>
    FORCEINLINE void Memzero() requires(TIsTrivial<U>::Value)
    {
        FMemory::Memzero(Elements, SizeInBytes());
    }

    /**
     * @brief Returns the index of an element if it is present in the array, or -1 if it is not found
     * @param Element Element to search for
     * @return The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SIZETYPE Find(const ElementType& Element) const
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start, *RESTRICT End = Start + NUM_ELEMENTS; Current != End; ++Current)
        {
            if (Element == *Current)
            {
                return static_cast<SIZETYPE>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief Returns the index of the element that satisfies the conditions of a comparator
     * @param Predicate Callable that compares an element in the array against some condition
     * @return The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindWithPredicate(PredicateType&& Predicate) const
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start, *RESTRICT End = Start + NUM_ELEMENTS; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief Returns the index of an element if it is present in the array, or -1 if it is not found
     * @param Element Element to search for
     * @return The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SIZETYPE FindLast(const ElementType& Element) const
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start + NUM_ELEMENTS, *RESTRICT End = Start; Current != End;)
        {
            --Current;
            if (Element == *Current)
            {
                return static_cast<SIZETYPE>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief Returns the index of the element that satisfies the conditions of a comparator
     * @param Predicate Callable that compares an element in the array against some condition
     * @return The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindLastWithPredicate(PredicateType&& Predicate) const
    {
        for (const ElementType* RESTRICT Start = Elements, *RESTRICT Current = Start + NUM_ELEMENTS, *RESTRICT End = Start; Current != End;)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief Check if an element exists in the array
     * @param Element Element to check for
     * @return Returns true if the element is found in the array and false if not
     */
    NODISCARD FORCEINLINE bool Contains(const ElementType& Element) const
    {
        return Find(Element) != INVALID_INDEX;
    }

    /**
     * @brief Check if an element that satisfies the conditions of a comparator exists in the array
     * @param Predicate Callable that compares an element in the array against some condition
     * @return Returns true if the comparator returned true for one element
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE bool ContainsWithPredicate(PredicateType&& Predicate) const
    {
        return FindWithPredicate(Forward<PredicateType>(Predicate)) != INVALID_INDEX;
    }

    /**
     * @brief Perform some function on each element in the array
     * @param Functor Callable that takes one element and performs some operation on it
     */
    template<class LambdaType>
    FORCEINLINE void Foreach(LambdaType&& Lambda)
    {
        for (ElementType* RESTRICT Current = Elements, *RESTRICT End = Current + NUM_ELEMENTS; Current != End; ++Current)
        {
            Lambda(*Current);
        }
    }

    /**
     * @brief Swap two elements with each other
     * @param FirstIndex Index to the first element to swap
     * @param SecondIndex Index to the second element to swap
     */
    FORCEINLINE void Swap(SIZETYPE FirstIndex, SIZETYPE SecondIndex)
    {
        CHECK(IsValidIndex(FirstIndex));
        CHECK(IsValidIndex(SecondIndex));
        ::Swap(Elements[FirstIndex], Elements[SecondIndex]);
    }

    /**
     * @brief Retrieve the data of the array
     * @return Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE ElementType* Data()
    {
        return Elements;
    }

    /**
     * @brief Retrieve the data of the array
     * @return Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const ElementType* Data() const
    {
        return Elements;
    }

public:

    /**
     * @brief Bracket-operator to retrieve an element at a certain index
     * @param Index Index of the element to retrieve
     * @return A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SIZETYPE Index)
    {
        CHECK(Index < NUM_ELEMENTS);
        return Elements[Index];
    }

    /**
     * @brief Bracket-operator to retrieve an element at a certain index
     * @param Index Index of the element to retrieve
     * @return A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SIZETYPE Index) const
    {
        CHECK(Index < NUM_ELEMENTS);
        return Elements[Index];
    }

    /**
     * @brief Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param Other Array to compare with
     * @return Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator==(const ArrayType& Other) const requires(TIsTArrayType<ArrayType>::Value)
    {
        return (NUM_ELEMENTS == FArrayContainerHelper::Size(Other)) ? ::CompareObjects<ElementType>(Elements, FArrayContainerHelper::Data(Other), NUM_ELEMENTS) : false;
    }

    /**
     * @brief Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param Other Array to compare with
     * @return Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator!=(const ArrayType& Other) const requires(TIsTArrayType<ArrayType>::Value)
    {
        return !(*this == Other);
    }

public:

    /**
     * @brief Retrieve the last index that can be used to retrieve an element from the array
     * @return Returns the index to the last element of the array
     */
    NODISCARD constexpr SIZETYPE LastElementIndex() const
    {
        return NUM_ELEMENTS - 1;
    }

    /**
     * @brief Returns the size of the container
     * @return The current size of the container
     */
    NODISCARD constexpr SIZETYPE Size() const
    {
        return NUM_ELEMENTS;
    }

    /**
     * @brief Returns the size of the container in bytes
     * @return The current size of the container in bytes
     */
    NODISCARD constexpr SIZETYPE SizeInBytes() const
    {
        return NUM_ELEMENTS * sizeof(ElementType);
    }

    /**
     * @brief Returns the capacity of the container
     * @return The current capacity of the container
     */
    NODISCARD constexpr SIZETYPE Capacity() const
    {
        return NUM_ELEMENTS;
    }

    /**
     * @brief Returns the capacity of the container in bytes
     * @return The current capacity of the container in bytes
     */
    NODISCARD constexpr SIZETYPE CapacityInBytes() const
    {
        return NUM_ELEMENTS * sizeof(ElementType);
    }

public:

    // Iterators
    NODISCARD FORCEINLINE IteratorType Iterator()
    {
        return IteratorType(*this, 0);
    }

    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const
    {
        return ConstIteratorType(*this, 0);
    }

    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator()
    {
        return ReverseIteratorType(*this, NUM_ELEMENTS);
    }

    NODISCARD ReverseConstIteratorType ConstReverseIterator() const
    {
        return ReverseConstIteratorType(*this, NUM_ELEMENTS);
    }

public:

    // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return ConstIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       { return IteratorType(*this, NUM_ELEMENTS); }
    NODISCARD FORCEINLINE ConstIteratorType end() const { return ConstIteratorType(*this, NUM_ELEMENTS); }

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
