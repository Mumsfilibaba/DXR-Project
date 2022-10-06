#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsTArrayType.h"
#include "Core/Templates/RemoveCV.h"
#include "Core/Templates/RemoveReference.h"
#include "Core/Templates/RemovePointer.h"
#include "Core/Templates/ContiguousContainerHelper.h"
#include "Core/Templates/DeclVal.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TArrayView - View of an array

template<typename T>
class TArrayView
{
public:
    using ElementType = T;
    using SizeType    = int32;

    static_assert(
        TIsSigned<SizeType>::Value,
        "TArrayView only supports a SizeType that's signed");

    /* Iterators */
    typedef TArrayIterator<TArrayView, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArrayView, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArrayView, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArrayView, const ElementType> ReverseConstIteratorType;

    enum : SizeType { INVALID_INDEX = -1 };

public:

    /**
     * @brief: Default construct an empty view 
     */
    FORCEINLINE TArrayView() noexcept
        : View(nullptr)
        , ViewSize(0)
    { }

    /**
     * @brief: Construct a view from an array of ArrayType
     * 
     * @param Container: Container to create view from (TArray, TArrayView, TStaticArray)
     */
    template<
        typename ContainerType,
        typename PureContainerType = typename TRemoveCV<typename TRemoveReference<ContainerType>::Type>::Type,
        typename = typename TEnableIf<TIsContiguousContainer<PureContainerType>::Value>::Type>
    FORCEINLINE TArrayView(ContainerType&& Container) noexcept
        : View(FContiguousContainerHelper::GetData(Forward<ContainerType>(Container)))
        , ViewSize(SizeType(FContiguousContainerHelper::GetSize(Forward<ContainerType>(Container))))
    { }

    /**
     * @brief: Construct a view from an array of initializer_list
     * 
     * @param InitList: initializer_list to create view from
     */
    FORCEINLINE TArrayView(std::initializer_list<ElementType> InitList) noexcept
        : View(FContiguousContainerHelper::GetData(InitList))
        , ViewSize(SizeType(FContiguousContainerHelper::GetSize(InitList)))
    { }

    /**
     * @brief: Construct a view from a raw-array
     *
     * @param InArray: Array to create view from
     * @param NumElements: Number of elements in the array
     */
    template<typename OtherElementType>
    FORCEINLINE TArrayView(OtherElementType* InArray, SizeType NumElements) noexcept
        : View(InArray)
        , ViewSize(NumElements)
    { }

    /**
     * @brief: Checks if an index is a valid index
     *
     * @return: Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SizeType Index) const noexcept
    {
        return (Index >= 0) && (Index < ViewSize);
    }

    /**
     * @brief: Check if the container contains any elements
     *
     * @return: Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ViewSize == 0);
    }

    /**
     * @brief: Retrieve the first element of the view
     *
     * @return: Returns a reference to the first element of the view
     */
    NODISCARD FORCEINLINE ElementType& FirstElement() const noexcept
    {
        CHECK(IsEmpty());
        return *GetData();
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the view
     */
    NODISCARD FORCEINLINE ElementType& LastElement() const noexcept
    {
        CHECK(IsEmpty());
        return *(GetData() + (ViewSize - 1));
    }

    /**
     * @brief: Retrieve a element at a certain index of the view
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& GetElementAt(SizeType Index) const noexcept
    {
        CHECK(Index < ViewSize);
        return *(GetData() + Index);
    }

    /**
     * @brief: Returns the index of an element if it is present in the array, or -1 if it is not found
     *
     * @param Element: Element to search for
     * @return: The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
    {
        const ElementType* RESTRICT CurrentAddress = GetData();
        const ElementType* RESTRICT EndAddress     = GetData() + ViewSize;
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
     * @brief: Returns the index of the element that satisfy the conditions of a comparator
     *
     * @param Predicate: Callable that compares an element in the array against some condition
     * @return: The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SizeType FindWithPredicate(PredicateType&& Predicate) const noexcept
    {
        const ElementType* RESTRICT CurrentAddress = GetData();
        const ElementType* RESTRICT EndAddress     = GetData() + ViewSize;
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
     * @brief: Returns the index of an element if it is present in the array, or -1 if it is not found
     *
     * @param Element: Element to search for
     * @return: The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SizeType FindLast(const ElementType& Element) const noexcept
    {
        const ElementType* RESTRICT CurrentAddress = GetData() + ViewSize;
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
     * @brief: Returns the index of the element that satisfy the conditions of a comparator
     *
     * @param Predicate: Callable that compares an element in the array against some condition
     * @return: The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SizeType FindLastWithPredicate(PredicateType&& Predicate) const noexcept
    {
        const ElementType* RESTRICT CurrentAddress = GetData() + ViewSize;
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
     * @brief: Check if an element exists in the array
     * 
     * @param Element: Element to check for
     * @return: Returns true if the element is found in the array and false if not
     */
    NODISCARD FORCEINLINE bool Contains(const ElementType& Element) const noexcept
    {
        return (Find(Element) != INVALID_INDEX);
    }

    /**
     * @brief: Check if an element that satisfies the conditions of a comparator exists in the array
     * 
     * @param Predicate: Callable that compares an element in the array against some condition
     * @return: Returns true if the comparator returned true for one element
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE bool ContainsWithPredicate(PredicateType&& Predicate) const noexcept
    {
        return (FindWithPredicate(Forward<PredicateType>(Predicate)) != INVALID_INDEX);
    }

    /**
     * @brief: Perform some function on each element in the array
     * 
     * @param Functor: Callable that takes one element and perform some operation on it
     */
    template<class FunctorType>
    FORCEINLINE void Foreach(FunctorType&& Functor) const
    {
        ElementType* RESTRICT CurrentAddress = GetData();
        ElementType* RESTRICT EndAddress     = GetData() + ViewSize;
        while (CurrentAddress != EndAddress)
        {
            Functor(*CurrentAddress);
            ++CurrentAddress;
        }
    }

    /**
     * @brief: Swap the contents of this view with another
     *
     * @param Other: The other view to swap with
     */
    FORCEINLINE void Swap(TArrayView& Other) noexcept
    {
        TArrayView Temp(Move(*this));
        *this = Move(Other);
        Other = Move(Temp);
    }

    /**
     * @brief: Fill the container with the specified value
     *
     * @param InputElement: Element to copy into all elements in the view
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ElementType* Elements = GetData();
        ::AssignElements(Elements, InputElement, GetSize());
    }

    /**
     * @brief: Sets the array to zero
     */
    template<typename U = ElementType>
    FORCEINLINE typename TEnableIf<TIsTrivial<U>::Value>::Type Memzero()
    {
        ElementType* Elements = GetData();
        FMemory::Memzero(Elements, SizeInBytes());
    }

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the view
     *
     * @return: Returns a the index to the last element of the view
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (ViewSize > 0) ? (ViewSize - 1) : 0;
    }

    /**
     * @brief: Returns the size of the container
     *
     * @return: The current size of the container
     */
    NODISCARD FORCEINLINE SizeType GetSize() const noexcept
    {
        return ViewSize;
    }

    /**
     * @brief: Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return GetSize() * sizeof(ElementType);
    }

    /**
     * @brief: Retrieve the data of the view
     *
     * @return: Returns a pointer to the data of the view
     */
    NODISCARD FORCEINLINE ElementType* GetData() const noexcept
    {
        return View;
    }

    /**
     * @brief: Create a sub-view
     * 
     * @param Offset: Offset into the view
     * @param NumElements: Number of elements to include in the view
     * @return: A new array-view pointing to the specified elements
     */
    NODISCARD FORCEINLINE TArrayView SubView(SizeType Offset, SizeType NumElements) const noexcept
    {
        CHECK((NumElements < ViewSize) && (Offset + NumElements < ViewSize));
        return TArrayView(View + Offset, NumElements);
    }

public:

    /**
     * @brief: Comparison operator that compares all elements in the view, which can be of any ArrayType qualified type
     *
     * @param RHS: Array to compare with
     * @return: Returns true if all elements are equal to each other
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
     * @brief: Comparison operator that compares all elements in the view, which can be of any ArrayType qualified type
     *
     * @param RHS: Array to compare with
     * @return: Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator!=(const ArrayType& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    /**
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SizeType Index) noexcept
    {
        return GetElementAt(Index);
    }

    /**
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        return GetElementAt(Index);
    }

    /**
     * @brief: Copy-assignment operator
     *
     * @param RHS: View to copy
     * @return: A reference to this container
     */
    FORCEINLINE TArrayView& operator=(const TArrayView& RHS) noexcept
    {
        View     = RHS.View;
        ViewSize = RHS.ViewSize;
        return *this;
    }

public:

    /**
     * @brief: Retrieve an iterator to the beginning of the view
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the view
     *
     * @return: A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an iterator to the beginning of the view
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the view
     *
     * @return: A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the view
     *
     * @return: A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the view
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the view
     *
     * @return: A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the view
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // STL Iterators

    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return StartIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return StartIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return EndIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return EndIterator(); }

private:
    ElementType* View;
    SizeType     ViewSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsTArrayType

template<typename T>
struct TIsTArrayType<TArrayView<T>>
{
    enum { Value = true };
};

template<typename T>
struct TIsContiguousContainer<TArrayView<T>>
{
    enum { Value = true };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// MakeArrayView

template<
    typename ContainerType,
    typename PureContainerType = typename TRemoveCV<typename TRemoveReference<ContainerType>::Type>::Type,
    typename = typename TEnableIf<TIsContiguousContainer<PureContainerType>::Value>::Type>
auto MakeArrayView(ContainerType&& Container)
{
    using ElementType = typename TRemovePointer<decltype(FContiguousContainerHelper::GetData(DeclVal<PureContainerType>()))>::Type;
    return TArrayView<ElementType>(Forward<ContainerType>(Container));
}

template<typename T>
auto MakeArrayView(std::initializer_list<T> InitList)
{
    return TArrayView<T>(InitList);
}

template<typename T>
auto MakeArrayView(T* Array, int32 Size)
{
    return TArrayView<T>(Array, Size);
}