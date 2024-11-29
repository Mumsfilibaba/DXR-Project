#pragma once
#include "Core/Containers/Iterator.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ArrayContainerHelper.h"

template<typename ElementType>
class TArrayView
{
public:
    typedef int32 SIZETYPE;
    static_assert(TIsSigned<SIZETYPE>::Value, "TArrayView only supports a SIZETYPE that's signed");

    typedef TArrayIterator<TArrayView, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArrayView, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArrayView, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArrayView, const ElementType> ReverseConstIteratorType;

    inline static constexpr SIZETYPE INVALID_INDEX = SIZETYPE(~0);

public:

    /** @brief Default construct an empty view */
    TArrayView() = default;

    /**
     * @brief Construct a view from a contiguous container (e.g., TArray, TArrayView, TStaticArray)
     * @param Container Container to create view from
     */
    template<
        typename ContainerType,
        typename PureContainerType = typename TRemoveCV<typename TRemoveReference<ContainerType>::Type>::Type>
    FORCEINLINE TArrayView(ContainerType&& Container) requires(TIsContiguousContainer<PureContainerType>::Value)
        : View(FArrayContainerHelper::Data(Forward<ContainerType>(Container)))
        , ViewSize(static_cast<SIZETYPE>(FArrayContainerHelper::Size(Forward<ContainerType>(Container))))
    {
    }

    /**
     * @brief Construct a view from an initializer_list
     * @param InitList initializer_list to create view from
     */
    FORCEINLINE TArrayView(std::initializer_list<ElementType> InitList)
        : View(FArrayContainerHelper::Data(InitList))
        , ViewSize(static_cast<SIZETYPE>(FArrayContainerHelper::Size(InitList)))
    {
    }

    /**
     * @brief Construct a view from a raw-array
     * @param InElements Array to create view from
     * @param NumElements Number of elements in the array
     */
    template<typename OtherElementType>
    FORCEINLINE TArrayView(OtherElementType* InElements, SIZETYPE NumElements)
        : View(InElements)
        , ViewSize(NumElements)
    {
        static_assert(TIsConvertible<typename TAddPointer<OtherElementType>::Type, typename TAddPointer<ElementType>::Type>::Value ||
            TIsSame<OtherElementType, ElementType>::Value, "OtherElementType must be convertible to ElementType");
    }

    /**
     * @brief Returns true if the address is within the view's range
     * @param Address Address to check.
     * @return Returns true if the address belongs to the view
     */
    NODISCARD FORCEINLINE bool CheckAddress(const ElementType* Address) const
    {
        if (!View || ViewSize == 0)
        {
            return false;
        }

        return Address >= View && Address < (View + ViewSize);
    }

    /**
     * @brief Checks if an index is a valid index
     * @return Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SIZETYPE Index) const
    {
        return Index >= 0 && Index < ViewSize;
    }

    /**
     * @brief Check if the view contains any elements
     * @return Returns true if the view is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return ViewSize == 0;
    }

    /**
     * @brief Retrieve the first element of the view
     * @return Returns a reference to the first element of the view
     */
    NODISCARD FORCEINLINE ElementType& FirstElement() const
    {
        CHECK(!IsEmpty());
        return View[0];
    }

    /**
     * @brief Retrieve the last element of the view
     * @return Returns a reference to the last element of the view
     */
    NODISCARD FORCEINLINE ElementType& LastElement() const
    {
        CHECK(!IsEmpty());
        return View[ViewSize - 1];
    }

    /**
     * @brief Returns the index of an element if it is present in the view, or INVALID_INDEX if it is not found
     * @param InElement Element to search for
     * @return The index of the element if found or INVALID_INDEX if not
     */
    NODISCARD FORCEINLINE SIZETYPE Find(const ElementType& InElement) const
    {
        return FindForward([&](const ElementType& Element)
        {
            return Element == InElement;
        });
    }

    /**
     * @brief Returns the index of the element that satisfies the conditions of a comparator
     * @param Predicate Callable that compares an element in the view against some condition
     * @return The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindWithPredicate(PredicateType&& Predicate) const
    {
        return FindForward(Forward<PredicateType>(Predicate));
    }

    /**
     * @brief Returns the index of the last occurrence of an element if it is present in the view, or INVALID_INDEX if it is not found
     * @param InElement Element to search for
     * @return The index of the element if found or INVALID_INDEX if not
     */
    NODISCARD FORCEINLINE SIZETYPE FindLast(const ElementType& InElement) const
    {
        return FindReverse([&](const ElementType& Element)
        {
            return Element == InElement;
        });
    }

    /**
     * @brief Returns the index of the last element that satisfies the conditions of a comparator
     * @param Predicate Callable that compares an element in the view against some condition
     * @return The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindLastWithPredicate(PredicateType&& Predicate) const
    {
        return FindReverse(Forward<PredicateType>(Predicate));
    }

    /**
     * @brief Check if an element exists in the view
     * @param Element Element to check for
     * @return Returns true if the element is found in the view and false if not
     */
    NODISCARD FORCEINLINE bool Contains(const ElementType& Element) const
    {
        return Find(Element) != INVALID_INDEX;
    }

    /**
     * @brief Check if an element that satisfies the conditions of a comparator exists in the view
     * @param Predicate Callable that compares an element in the view against some condition
     * @return Returns true if the comparator returned true for one element
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE bool ContainsWithPredicate(PredicateType&& Predicate) const
    {
        return FindWithPredicate(Forward<PredicateType>(Predicate)) != INVALID_INDEX;
    }

    /**
     * @brief Perform some function on each element in the view
     * @param Lambda Callable that takes one element and performs some operation on it
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
     * @brief Swap the contents of this view with another
     * @param Other The other view to swap with
     */
    FORCEINLINE void Swap(TArrayView& Other)
    {
        TArrayView Temp(Move(*this));
        *this = Move(Other);
        Other = Move(Temp);
    }

    /**
     * @brief Fill the view with the specified value
     * @param InputElement Element to copy into all elements in the view
     */
    FORCEINLINE void Fill(const ElementType& InputElement)
    {
        ::AssignObjects(View, InputElement, ViewSize);
    }

    /**
     * @brief Sets the view's memory to zero
     */
    template<typename U = ElementType>
    FORCEINLINE void Memzero() requires(TIsTrivial<U>::Value)
    {
        FMemory::Memzero(View, SizeInBytes());
    }

    /**
     * @brief Retrieve the last index that can be used to retrieve an element from the view
     * @return Returns the index to the last element of the view
     */
    NODISCARD FORCEINLINE SIZETYPE LastElementIndex() const
    {
        return ViewSize > 0 ? ViewSize - 1 : 0;
    }

    /**
     * @brief Returns the size of the view
     * @return The current size of the view
     */
    NODISCARD FORCEINLINE SIZETYPE Size() const
    {
        return ViewSize;
    }

    /**
     * @brief Returns the size of the view in bytes
     * @return The current size of the view in bytes
     */
    NODISCARD FORCEINLINE SIZETYPE SizeInBytes() const
    {
        return ViewSize * sizeof(ElementType);
    }

    /**
     * @brief Retrieve the data of the view
     * @return Returns a pointer to the data of the view
     */
    NODISCARD FORCEINLINE ElementType* Data() const
    {
        return View;
    }

    /**
     * @brief Create a sub-view
     * @param Offset Offset into the view
     * @param NumElements Number of elements to include in the view
     * @return A new array-view pointing to the specified elements
     */
    NODISCARD FORCEINLINE TArrayView SubView(SIZETYPE Offset, SIZETYPE NumElements) const
    {
        CHECK(Offset >= 0 && NumElements >= 0 && (Offset + NumElements) <= ViewSize);
        return TArrayView(View + Offset, NumElements);
    }

public:

    /**
     * @brief Comparison operator that compares all elements in the view, which can be of any ArrayType qualified type
     * @param RHS Array to compare with
     * @return Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator==(const ArrayType& RHS) const requires(TIsTArrayType<ArrayType>::Value)
    {
        if (ViewSize != FArrayContainerHelper::Size(RHS))
        {
            return false;
        }

        return ::CompareObjects<ElementType>(View, FArrayContainerHelper::Data(RHS), ViewSize);
    }

    /**
     * @brief Comparison operator that compares all elements in the view, which can be of any ArrayType qualified type
     * @param RHS Array to compare with
     * @return Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator!=(const ArrayType& RHS) const requires(TIsTArrayType<ArrayType>::Value)
    {
        return !(*this == RHS);
    }

    /**
     * @brief Bracket-operator to retrieve an element at a certain index
     * @param Index Index of the element to retrieve
     * @return A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SIZETYPE Index)
    {
        CHECK(Index < ViewSize);
        return View[Index];
    }

    /**
     * @brief Bracket-operator to retrieve an element at a certain index
     * @param Index Index of the element to retrieve
     * @return A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SIZETYPE Index) const
    {
        CHECK(Index < ViewSize);
        return View[Index];
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
        return ReverseIteratorType(*this, ViewSize);
    }

    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseIterator() const
    {
        return ReverseConstIteratorType(*this, ViewSize);
    }

public:

    // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return ConstIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       { return IteratorType(*this, ViewSize); }
    NODISCARD FORCEINLINE ConstIteratorType end() const { return ConstIteratorType(*this, ViewSize); }

private:
    template<typename PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindForward(PredicateType&& Predicate) const
    {
        for (const ElementType* RESTRICT Current = View, *RESTRICT End = View + ViewSize; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(Current - View);
            }
        }

        return INVALID_INDEX;
    }

    template<typename PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindReverse(PredicateType&& Predicate) const
    {
        for (const ElementType* RESTRICT Current = View + ViewSize, *RESTRICT End = View; Current != End;)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(Current - View);
            }
        }

        return INVALID_INDEX;
    }

private:
    ElementType* View     = nullptr;
    SIZETYPE     ViewSize = 0;
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

/**
 * @brief Creates an array view from a contiguous container.
 * @tparam ContainerType The type of the container.
 * @param Container The container to create a view from.
 * @return A TArrayView pointing to the container's data.
 */
template<
    typename ContainerType,
    typename PureContainerType = typename TRemoveCV<typename TRemoveReference<ContainerType>::Type>::Type>
auto MakeArrayView(ContainerType&& Container) requires(TIsContiguousContainer<PureContainerType>::Value)
{
    using ElementType = typename TRemovePointer<decltype(FArrayContainerHelper::Data(::DeclVal<PureContainerType>()))>::Type;
    return TArrayView<ElementType>(Forward<ContainerType>(Container));
}

/**
 * @brief Creates an array view from an initializer list.
 * @tparam T The type of elements.
 * @param InitList The initializer list to create a view from.
 * @return A TArrayView pointing to the initializer list's data.
 */
template<typename T>
auto MakeArrayView(std::initializer_list<T> InitList)
{
    return TArrayView<T>(InitList);
}

/**
 * @brief Creates an array view from a raw array.
 * @tparam T The type of elements.
 * @param Array The raw array.
 * @param Size The number of elements in the array.
 * @return A TArrayView pointing to the raw array's data.
 */
template<typename T>
auto MakeArrayView(T* Array, int32 Size)
{
    return TArrayView<T>(Array, Size);
}
