#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsTArrayType.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TArrayView - View of an array similar to std::span

template<typename T>
class TArrayView
{
public:

    using ElementType = T;
    using SizeType = int32;

    /* Iterators */
    typedef TArrayIterator<TArrayView, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArrayView, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArrayView, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArrayView, const ElementType> ReverseConstIteratorType;

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
     * @param InArray: Array to create view from
     */
    template<typename ArrayType, typename = typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type>
    FORCEINLINE explicit TArrayView(ArrayType& InArray) noexcept
        : View(InArray.Data())
        , ViewSize(InArray.Size())
    { }

    /**
     * @brief: Construct a view from an array of ArrayType
     *
     * @param InArray: Array to create view from
     */
    template<typename ArrayType, typename = typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type>
    FORCEINLINE explicit TArrayView(const ArrayType& InArray) noexcept
        : View(InArray.Data())
        , ViewSize(InArray.Size())
    { }

    /**
     * @brief: Construct a view from a bounded array
     *
     * @param InArray: Array to create view from
     */
    template<SizeType N>
    FORCEINLINE explicit TArrayView(ElementType(&InArray)[N]) noexcept
        : View(InArray)
        , ViewSize(N)
    { }

    /**
     * @brief: Construct a view from a raw-array
     *
     * @param InArray: Array to create view from
     * @param NumElements: Number of elements in the array
     */
    FORCEINLINE explicit TArrayView(ElementType* InArray, SizeType NumElements) noexcept
        : View(InArray)
        , ViewSize(NumElements)
    { }

    /**
     * @brief: Copy-constructor
     * 
     * @param Other: Array to copy from
     */
    FORCEINLINE TArrayView(const TArrayView& Other) noexcept
        : View(Other.View)
        , ViewSize(Other.ViewSize)
    { }

    /**
     * @brief: Move-constructor
     *
     * @param Other: Array to move from
     */
    FORCEINLINE TArrayView(TArrayView&& Other) noexcept
        : View(Other.View)
        , ViewSize(Other.ViewSize)
    {
        Other.View = nullptr;
        Other.ViewSize = 0;
    }

    /**
     * @brief: Check if the view contains elements or not
     * 
     * @return: Returns true if the view is empty
     */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ViewSize == 0);
    }

    /**
     * @brief: Retrieve the first element of the view
     *
     * @return: Returns a reference to the first element of the view
     */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert(IsEmpty());
        return Data()[0];
    }

    /**
     * @brief: Retrieve the first element of the view
     *
     * @return: Returns a reference to the first element of the view
     */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert(IsEmpty());
        return Data()[0];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the view
     */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert(IsEmpty());
        return Data()[ViewSize - 1];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the view
     */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert(IsEmpty());
        return Data()[ViewSize - 1];
    }

    /**
     * @brief: Retrieve a element at a certain index of the view
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE ElementType& At(SizeType Index) noexcept
    {
        Assert(Index < ViewSize);
        return Data()[Index];
    }

    /**
     * @brief: Retrieve a element at a certain index of the view
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const ElementType& At(SizeType Index) const noexcept
    {
        Assert(Index < ViewSize);
        return Data()[Index];
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
        ElementType* Elements = Data();
        FillRange(Elements, InputElement, Size());
    }

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the view
     *
     * @return: Returns a the index to the last element of the view
     */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return ViewSize > 0 ? ViewSize - 1 : 0;
    }

    /**
     * @brief: Returns the size of the container
     *
     * @return: The current size of the container
     */
    FORCEINLINE SizeType Size() const noexcept
    {
        return ViewSize;
    }

    /**
     * @brief: Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof(ElementType);
    }

    /**
     * @brief: Retrieve the data of the view
     *
     * @return: Returns a pointer to the data of the view
     */
    FORCEINLINE ElementType* Data() noexcept
    {
        return View;
    }

    /**
     * @brief: Retrieve the data of the view
     *
     * @return: Returns a pointer to the data of the view
     */
    FORCEINLINE const ElementType* Data() const noexcept
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
    FORCEINLINE TArrayView SubView(SizeType Offset, SizeType NumElements) const noexcept
    {
        Assert((NumElements < ViewSize) && (Offset + NumElements < ViewSize));
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
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator==(const ArrayType& RHS) const noexcept
    {
        if (Size() != RHS.Size())
        {
            return false;
        }

        return CompareRange<ElementType>(Data(), RHS.Data(), Size());
    }

    /**
     * @brief: Comparison operator that compares all elements in the view, which can be of any ArrayType qualified type
     *
     * @param RHS: Array to compare with
     * @return: Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator!=(const ArrayType& RHS) const noexcept
    {
        return !(*this == RHS);
    }

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
     * @brief: Copy-assignment operator
     *
     * @param RHS: View to copy
     * @return: A reference to this container
     */
    FORCEINLINE TArrayView& operator=(const TArrayView& RHS) noexcept
    {
        View = RHS.View;
        ViewSize = RHS.ViewSize;
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: View to move
     * @return: A reference to this container
     */
    FORCEINLINE TArrayView& operator=(TArrayView&& RHS) noexcept
    {
        if (this != &RHS)
        {
            View         = RHS.View;
            ViewSize     = RHS.ViewSize;
            RHS.View     = nullptr;
            RHS.ViewSize = 0;
        }

        return *this;
    }

public:

    /**
     * @brief: Retrieve an iterator to the beginning of the view
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the view
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an iterator to the beginning of the view
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the view
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the view
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the view
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the view
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the view
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /**
     * @brief: STL start iterator, same as TArrayView::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * @brief: STL end iterator, same as TArrayView::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * @brief: STL start iterator, same as TArrayView::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * @brief: STL end iterator, same as TArrayView::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:
    ElementType* View;
    SizeType ViewSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Enable TArrayType

template<typename T>
struct TIsTArrayType<TArrayView<T>>
{
    enum
    {
        Value = true
    };
};
