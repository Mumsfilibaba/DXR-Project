#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsCopyable.h"
#include "Core/Templates/IsMovable.h"
#include "Core/Templates/AddReference.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/Not.h"
#include "Core/Templates/IsTArrayType.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A fixed size array similar to std::array

template<typename T, int32 NumElements>
struct TStaticArray
{
public:

    using ElementType = T;
    using SizeType = int32;

    typedef TArrayIterator<TStaticArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TStaticArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticArray, const ElementType> ReverseConstIteratorType;

    static_assert(NumElements > 0, "The number of elements has to be more than zero");

    /**
     * @brief: Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        return Elements[0];
    }

    /**
     * @brief: Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        return Elements[0];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        return Elements[NumElements - 1];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        return Elements[NumElements - 1];
    }

    /**
     * @brief: Retrieve a element at a certain index of the array
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE ElementType& At(SizeType Index) noexcept
    {
        Check(Index < NumElements);
        return Elements[Index];
    }

    /**
     * @brief: Retrieve a element at a certain index of the array
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const ElementType& At(SizeType Index) const noexcept
    {
        Check(Index < NumElements);
        return Elements[Index];
    }

    /**
     * @brief: Fill the container with the specified value
     *
     * @param InputElement: Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        for (ElementType& Element : *this)
        {
            Element = InputElement;
        }
    }

    /**
     * @brief: Swap the contents of this array with another
     *
     * @param Other: The other array to swap with
     */
    FORCEINLINE void Swap(TStaticArray& Other) noexcept
    {
        TStaticArray TempArray(Move(*this));
        *this = Move(Other);
        Other = Move(TempArray);
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE ElementType* Data() noexcept
    {
        return Elements;
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const ElementType* Data() const noexcept
    {
        return Elements;
    }

public:

    /**
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE ElementType& operator[](SizeType Index) noexcept
    {
        return At(Index);
    }

    /**
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        return At(Index);
    }

    /**
     * @brief: Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     *
     * @param RHS: Array to compare with
     * @return: Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator==(const ArrayType& RHS) const noexcept
    {
        if (Size() != RHS.Size())
        {
            return false;
        }

        return CompareRange<ElementType>(Data(), RHS.Data(), Size());
    }

    /**
     * @brief: Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     *
     * @param RHS: Array to compare with
     * @return: Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator!=(const ArrayType& RHS) const noexcept
    {
        return !(*this == RHS);
    }

public:

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the array
     *
     * @return: Returns a the index to the last element of the array
     */
    constexpr SizeType LastElementIndex() const noexcept
    {
        return NumElements - 1;
    }

    /**
     * @brief: Returns the size of the container
     *
     * @return: The current size of the container
     */
    constexpr SizeType Size() const noexcept
    {
        return NumElements;
    }

    /**
     * @brief: Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    constexpr SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof(ElementType);
    }

    /**
     * @brief: Returns the capacity of the container
     *
     * @return: The current capacity of the container
     */
    constexpr SizeType Capacity() const noexcept
    {
        return NumElements;
    }

    /**
     * @brief: Returns the capacity of the container in bytes
     *
     * @return: The current capacity of the container in bytes
     */
    constexpr SizeType CapacityInBytes() const noexcept
    {
        return Capacity() * sizeof(ElementType);
    }

public:

    /**
     * @brief: Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /**
     * @brief: STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * @brief: STL end iterator, same as TArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * @brief: STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * @brief: STL end iterator, same as TArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

public:
    ElementType Elements[NumElements];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Enable TArrayType

template<typename T, int32 NumElements>
struct TIsTArrayType<TStaticArray<T, NumElements>>
{
    enum
    {
        Value = true
    };
};
