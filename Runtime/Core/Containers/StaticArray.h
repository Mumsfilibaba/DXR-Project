#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsCopyable.h"
#include "Core/Templates/IsMovable.h"
#include "Core/Templates/AddReference.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/Not.h"
#include "Core/Templates/IsTArrayType.h"
#include "Core/Templates/ContiguousContainerHelper.h"

template<
    typename T,
    int32 NUM_ELEMENTS>
struct TStaticArray
{
    using ElementType = T;
    using SizeType    = int32;

    static_assert(
        NUM_ELEMENTS > 0,
        "TStaticArray does not support a zero element count");
    static_assert(
        TIsSigned<SizeType>::Value,
        "TStaticArray only supports a SizeType that's signed");

    typedef TArrayIterator<TStaticArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TStaticArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticArray, const ElementType> ReverseConstIteratorType;

    enum : SizeType { INVALID_INDEX = -1 };

public:

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
     * @brief       - Retrieve a element at a certain index of the array
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& GetElementAt(SizeType Index) noexcept
    {
        CHECK(Index < NUM_ELEMENTS);
        return Elements[Index];
    }

    /**
     * @brief       - Retrieve a element at a certain index of the array
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& GetElementAt(SizeType Index) const noexcept
    {
        CHECK(Index < NUM_ELEMENTS);
        return Elements[Index];
    }

    /**
     * @brief              - Fill the container with the specified value
     * @param InputElement - Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ::AssignElements(Elements, InputElement, GetSize());
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
        const ElementType* RESTRICT CurrentAddress = GetData();
        const ElementType* RESTRICT EndAddress     = GetData() + GetSize();
        while (CurrentAddress != EndAddress)
        {
            if (Element == *CurrentAddress)
            {
                return static_cast<SizeType>(CurrentAddress - GetData());
            }

            ++CurrentAddress;
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
        const ElementType* RESTRICT CurrentAddress = GetData();
        const ElementType* RESTRICT EndAddress     = GetData() + GetSize();
        while (CurrentAddress != EndAddress)
        {
            if (Predicate(*CurrentAddress))
            {
                return static_cast<SizeType>(CurrentAddress - GetData());
            }

            ++CurrentAddress;
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
        const ElementType* RESTRICT CurrentAddress = GetData() + GetSize();
        const ElementType* RESTRICT EndAddress     = GetData();
        while (CurrentAddress != EndAddress)
        {
            --CurrentAddress;
            if (Element == *CurrentAddress)
            {
                return static_cast<SizeType>(CurrentAddress - GetData());
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
        const ElementType* RESTRICT CurrentAddress = GetData() + GetSize();
        const ElementType* RESTRICT EndAddress     = GetData();
        while (CurrentAddress != EndAddress)
        {
            --CurrentAddress;
            if (Predicate(*CurrentAddress))
            {
                return static_cast<SizeType>(CurrentAddress - GetData());
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
        return (Find(Element) != INVALID_INDEX);
    }

    /**
     * @brief           - Check if an element that satisfies the conditions of a comparator exists in the array
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - Returns true if the comparator returned true for one element
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE bool ContainsWithPredicate(PredicateType&& Predicate) const noexcept
    {
        return (FindWithPredicate(Forward<PredicateType>(Predicate)) != INVALID_INDEX);
    }

    /**
     * @brief         - Perform some function on each element in the array
     * @param Functor - Callable that takes one element and perform some operation on it
     */
    template<class FunctorType>
    FORCEINLINE void Foreach(FunctorType&& Functor)
    {
        ElementType* RESTRICT CurrentAddress = GetData();
        ElementType* RESTRICT EndAddress     = GetData() + GetSize();
        while (CurrentAddress != EndAddress)
        {
            Functor(*CurrentAddress);
            ++CurrentAddress;
        }
    }

    /**
     * @brief       - Swap the contents of this array with another
     * @param Other - The other array to swap with
     */
    FORCEINLINE void Swap(TStaticArray& Other) noexcept
    {
        TStaticArray TempArray(Move(*this));
        *this = Move(Other);
        Other = Move(TempArray);
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE ElementType* GetData() noexcept
    {
        return Elements;
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const ElementType* GetData() const noexcept
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
        return GetElementAt(Index);
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        return GetElementAt(Index);
    }

    /**
     * @brief     - Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param RHS - Array to compare with
     * @return    - Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator==(const ArrayType& RHS) const noexcept
    {
        if (GetSize() != RHS.GetSize())
        {
            return false;
        }

        return ::CompareElements<ElementType>(GetData(), RHS.GetData(), GetSize());
    }

    /**
     * @brief     - Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param RHS - Array to compare with
     * @return    - Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator!=(const ArrayType& RHS) const noexcept
    {
        return !(*this == RHS);
    }

public:

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns a the index to the last element of the array
     */
    NODISCARD CONSTEXPR SizeType LastElementIndex() const noexcept
    {
        return NUM_ELEMENTS - 1;
    }

    /**
     * @brief  - Returns the size of the container
     * @return - The current size of the container
     */
    NODISCARD CONSTEXPR SizeType GetSize() const noexcept
    {
        return NUM_ELEMENTS;
    }

    /**
     * @brief  - Returns the size of the container in bytes
     * @return - The current size of the container in bytes
     */
    NODISCARD CONSTEXPR SizeType SizeInBytes() const noexcept
    {
        return GetSize() * sizeof(ElementType);
    }

    /**
     * @brief  - Returns the capacity of the container
     * @return - The current capacity of the container
     */
    NODISCARD CONSTEXPR SizeType GetCapacity() const noexcept
    {
        return NUM_ELEMENTS;
    }

    /**
     * @brief  - Returns the capacity of the container in bytes
     * @return - The current capacity of the container in bytes
     */
    NODISCARD CONSTEXPR SizeType CapacityInBytes() const noexcept
    {
        return GetCapacity() * sizeof(ElementType);
    }

public:

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an iterator to the end of the array
     * @return - A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, GetSize());
    }

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an iterator to the end of the array
     * @return - A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, GetSize());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, GetSize());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the start of the array
     * @return - A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD  ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, GetSize());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the start of the array
     * @return - A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return StartIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return StartIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return EndIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return EndIterator(); }

public:
    ElementType Elements[NUM_ELEMENTS];
};


template<
    typename T,
    int32 NUM_ELEMENTS>
struct TIsTArrayType<TStaticArray<T, NUM_ELEMENTS>>
{
    enum { Value = true };
};

template<
    typename T,
    int32 NUM_ELEMENTS>
struct TIsContiguousContainer<TStaticArray<T, NUM_ELEMENTS>>
{
    enum { Value = true };
};
