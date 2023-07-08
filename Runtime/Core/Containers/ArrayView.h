#pragma once
#include "Iterator.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ArrayContainerHelper.h"

template<typename ElementType>
class TArrayView
{
public:
    using SizeType = int32;
    static_assert(TIsSigned<SizeType>::Value, "TArrayView only supports a SizeType that's signed");

    typedef TArrayIterator<TArrayView, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArrayView, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArrayView, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArrayView, const ElementType> ReverseConstIteratorType;

    enum : SizeType
    { 
        INVALID_INDEX = -1 
    };

public:

    /** @brief - Default construct an empty view */
    TArrayView() = default;

    /**
     * @brief           - Construct a view from an array of ArrayType
     * @param Container - Container to create view from (TArray, TArrayView, TStaticArray)
     */
    template<
        typename ContainerType,
        typename PureContainerType = typename TRemoveCV<typename TRemoveReference<ContainerType>::Type>::Type>
    FORCEINLINE TArrayView(ContainerType&& Container) noexcept requires(TIsContiguousContainer<PureContainerType>::Value)
        : View(FArrayContainerHelper::Data(::Forward<ContainerType>(Container)))
        , ViewSize(static_cast<SizeType>(FArrayContainerHelper::Size(::Forward<ContainerType>(Container))))
    {
    }

    /**
     * @brief          - Construct a view from an array of initializer_list
     * @param InitList - initializer_list to create view from
     */
    FORCEINLINE TArrayView(std::initializer_list<ElementType> InitList) noexcept
        : View(FArrayContainerHelper::Data(InitList))
        , ViewSize(static_cast<SizeType>(FArrayContainerHelper::Size(InitList)))
    {
    }

    /**
     * @brief             - Construct a view from a raw-array
     * @param InElements  - Array to create view from
     * @param NumElements - Number of elements in the array
     */
    template<typename OtherElementType>
    FORCEINLINE TArrayView(OtherElementType* InElements, SizeType NumElements) noexcept
        : View(InElements)
        , ViewSize(NumElements)
    {
    }

    /**
     * @brief         - Checks that the pointer is a part of the array
     * @param Address - Address to check.
     * @return        - Returns true if the address belongs to the array
     */
    NODISCARD FORCEINLINE bool CheckAddress(const ElementType* Address) const noexcept
    {
        return (Address >= View) && (Address < (View + ViewSize));
    }

    /**
     * @brief  - Checks if an index is a valid index
     * @return - Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SizeType Index) const noexcept
    {
        return (Index >= 0) && (Index < ViewSize);
    }

    /**
     * @brief  - Check if the container contains any elements
     * @return - Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return ViewSize == 0;
    }

    /**
     * @brief  - Retrieve the first element of the view
     * @return - Returns a reference to the first element of the view
     */
    NODISCARD FORCEINLINE ElementType& FirstElement() const noexcept
    {
        CHECK(!IsEmpty());
        return View[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the view
     */
    NODISCARD FORCEINLINE ElementType& LastElement() const noexcept
    {
        CHECK(!IsEmpty());
        return View[ViewSize - 1];
    }

    /**
     * @brief         - Returns the index of an element if it is present in the array, or -1 if it is not found
     * @param Element - Element to search for
     * @return        - The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
    {
        for (const ElementType* RESTRICT Current = View, *RESTRICT End = View + ViewSize; Current != End; ++Current)
        {
            if (Element == *Current)
            {
                return static_cast<SizeType>(Current - View);
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
        for (const ElementType* RESTRICT Current = View, *RESTRICT End = View + ViewSize; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - View);
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
        for (const ElementType* RESTRICT Current = View + ViewSize, *RESTRICT End = View; Current != End;)
        {
            --Current;
            if (Element == *Current)
            {
                return static_cast<SizeType>(Current - End);
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
        for (const ElementType* RESTRICT Current = View + ViewSize, *RESTRICT End = View; Current != End;)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - End);
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
     * @brief        - Perform some function on each element in the array
     * @param Lambda - Callable that takes one element and perform some operation on it
     */
    template<class LambdaType>
    FORCEINLINE void Foreach(LambdaType&& Lambda) const
    {
        for (const ElementType* RESTRICT Current = View, *RESTRICT End = Current + ViewSize; Current != End; ++Current)
        {
            Lambda(*Current);
        }
    }

    /**
     * @brief       - Swap the contents of this view with another
     * @param Other - The other view to swap with
     */
    FORCEINLINE void Swap(TArrayView& Other) noexcept
    {
        TArrayView Temp(::Move(*this));
        *this = ::Move(Other);
        Other = ::Move(Temp);
    }

    /**
     * @brief              - Fill the container with the specified value
     * @param InputElement - Element to copy into all elements in the view
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ::AssignObjects(View, InputElement, ViewSize);
    }

    /**
     * @brief - Sets the array to zero
     */
    template<typename U = ElementType>
    FORCEINLINE void Memzero() requires(TIsTrivial<U>::Value)
    {
        FMemory::Memzero(View, SizeInBytes());
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the view
     * @return - Returns a the index to the last element of the view
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (ViewSize > 0) ? (ViewSize - 1) : 0;
    }

    /**
     * @brief  - Returns the size of the container
     * @return - The current size of the container
     */
    NODISCARD FORCEINLINE SizeType Size() const noexcept
    {
        return ViewSize;
    }

    /**
     * @brief  - Returns the size of the container in bytes
     * @return - The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return ViewSize * sizeof(ElementType);
    }

    /**
     * @brief  - Retrieve the data of the view
     * @return - Returns a pointer to the data of the view
     */
    NODISCARD FORCEINLINE ElementType* Data() const noexcept
    {
        return View;
    }

    /**
     * @brief             - Create a sub-view
     * @param Offset      - Offset into the view
     * @param NumElements - Number of elements to include in the view
     * @return            - A new array-view pointing to the specified elements
     */
    NODISCARD FORCEINLINE TArrayView SubView(SizeType Offset, SizeType NumElements) const noexcept
    {
        CHECK(NumElements < ViewSize && (Offset + NumElements) < ViewSize);
        return TArrayView(View + Offset, NumElements);
    }

public:

    /**
     * @brief     - Comparison operator that compares all elements in the view, which can be of any ArrayType qualified type
     * @param RHS - Array to compare with
     * @return    - Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator==(const ArrayType& RHS) const noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        if (ViewSize != RHS.ViewSize)
        {
            return false;
        }

        return ::CompareObjects<ElementType>(View, FArrayContainerHelper::Data(RHS), ViewSize);
    }

    /**
     * @brief     - Comparison operator that compares all elements in the view, which can be of any ArrayType qualified type
     * @param RHS - Array to compare with
     * @return    - Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator!=(const ArrayType& RHS) const noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        return !(*this == RHS);
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SizeType Index) noexcept
    {
        CHECK(Index < ViewSize);
        return View[Index];
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        CHECK(Index < ViewSize);
        return View[Index];
    }

public: // Iterators

    /**
     * @brief  - Retrieve an iterator to the beginning of the view
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType Iterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an iterator to the beginning of the view
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the view
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator() noexcept
    {
        return ReverseIteratorType(*this, ViewSize);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the view
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, ViewSize);
    }

public: // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return ConstIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return IteratorType(*this, ViewSize); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return ConstIteratorType(*this, ViewSize); }

private:
    ElementType* View{nullptr};
    SizeType     ViewSize{0};
};


template<typename T>
struct TIsTArrayType<TArrayView<T>>
{
    inline static constexpr bool Value = true;
};

template<typename T>
struct TIsContiguousContainer<TArrayView<T>>
{
    inline static constexpr bool Value = true;
};


template<
    typename ContainerType,
    typename PureContainerType = typename TRemoveCV<typename TRemoveReference<ContainerType>::Type>::Type>
auto MakeArrayView(ContainerType&& Container) requires(TIsContiguousContainer<PureContainerType>::Value)
{
    using ElementType = typename TRemovePointer<decltype(FArrayContainerHelper::Data(::DeclVal<PureContainerType>()))>::Type;
    return TArrayView<ElementType>(::Forward<ContainerType>(Container));
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
