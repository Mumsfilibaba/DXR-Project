#pragma once
#include "Allocators.h"
#include "UniquePtr.h"
#include "ArrayView.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsPointer.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/Not.h"
#include "Core/Templates/IsReallocatable.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TArray - Dynamic Array similar to std::vector

template<typename T, typename AllocatorType = TDefaultArrayAllocator<T>>
class TArray
{
public:

    using ElementType = T;
    using SizeType    = int32;

    /* Iterators */
    typedef TArrayIterator<TArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArray, const ElementType> ReverseConstIteratorType;

    /** 
     * @brief: Default constructor
     */
    FORCEINLINE TArray() noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    { }

    /** 
     * @brief: Constructor that default creates a certain number of elements 
     *
     * @param InSize: Number of elements to construct
     */
    FORCEINLINE explicit TArray(SizeType InSize) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        EmptyConstruct(InSize);
    }

    /**
     * @brief: Constructor that Allocates the specified amount of elements, and initializes them to the same value 
     * 
     * @param InSize: Number of elements to construct
     * @param Element: Element to copy into all positions of the array
     */
    FORCEINLINE explicit TArray(SizeType InSize, const ElementType& Element) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        EmptyConstructFrom(InSize, Element);
    }

    /**
     * @brief: Constructor that creates an array from a raw pointer array 
     * 
     * @param InputArray: Pointer to the start of the array to copy from
     * @param NumElements: Number of elements in 'InputArray', which also is the resulting size of the constructed array
     */
    template<typename OtherElementType>
    FORCEINLINE explicit TArray(const OtherElementType* InputArray, SizeType NumElements) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(InputArray, NumElements);
    }

    /** 
     * Constructor that creates an array from an std::initializer_list
     * 
     * @param InList: Initializer list containing all elements to construct the array from
     */
    FORCEINLINE TArray(std::initializer_list<ElementType> InList) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(GetInitializerListData(InList) ,GetInitializerListSize(InList));
    }

    /** 
     * Copy-constructor 
     * 
     * @param Other: Array to copy from
     */
    FORCEINLINE TArray(const TArray& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(Other.Data(), Other.Size());
    }

    /**
     * @brief: Copy-constructor from another type of array
     *
     * @param Other: Array to copy from
     */
    template<typename ArrayType, typename = typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type>
    FORCEINLINE explicit TArray(const ArrayType& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(Other.Data(), Other.Size());
    }

    /** 
     * Move-constructor 
     * 
     * @param Other: Array to move elements from
     */
    FORCEINLINE TArray(TArray&& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        MoveFrom(Forward<TArray>(Other));
    }

    /** 
     * Destructor
     */
    FORCEINLINE ~TArray()
    {
        MakeEmpty();
    }

    /**
     * @brief: Destruct all elements of the array and deallocates the memory 
     */
    FORCEINLINE void MakeEmpty() noexcept
    {
        if (ArraySize)
        {
            DestructRange<ElementType>(Data(), ArraySize);
            ArraySize = 0;
        }

        Allocator.Free();
        ArrayCapacity = 0;
    }

    /** 
     * Clear all elements of the container, but does not deallocate the memory
     */
    FORCEINLINE void Clear() noexcept
    {
        DestructRange<ElementType>(Data(), ArraySize);
        ArraySize = 0;
    }

    /**
     * @brief: Resets the container, but does not deallocate the memory. Takes an optional parameter to 
     * default construct a new amount of elements.
     * 
     * @param NewSize: Number of elements to construct
     */
    FORCEINLINE void Reset(SizeType NewSize = 0) noexcept
    {
        DestructRange<ElementType>(Data(), ArraySize);

        if (NewSize)
        {
            EmptyConstruct(NewSize);
        }
        else
        {
            ArraySize = 0;
        }
    }

    /** 
     * Resets the container, but does not deallocate the memory. Takes an optional parameter to
     * default construct a new amount of elements from a single element. 
     * 
     * @param NewSize: Number of elements to construct
     * @param Element: Element to copy-construct from
     */
    FORCEINLINE void Reset(SizeType NewSize, const ElementType& Element) noexcept
    {
        DestructRange<ElementType>(Data(), ArraySize);

        if (NewSize)
        {
            EmptyConstructFrom(NewSize, Element);
        }
        else
        {
            ArraySize = 0;
        }
    }

    /** 
     * Resets the container, but does not deallocate the memory. Takes in pointer to copy-construct elements from.
     * 
     * @param InputArray: Array to copy-construct from
     * @param NumElements: Number of elements from
     */
    FORCEINLINE void Reset(const ElementType* InputArray, SizeType NumElements) noexcept
    {
        Assert((InputArray != nullptr) && (NumElements > 0));

        if (InputArray != Data())
        {
            DestructRange<ElementType>(Data(), ArraySize);

            if (NumElements)
            {
                CopyConstructFrom(InputArray, NumElements);
            }
            else
            {
                ArraySize = 0;
            }
        }
    }

    /** 
     * Resets the container, but does not deallocate the memory. Creates a new array from
     * another array which can be of another type of array.
     * 
     * @param InputArray: Array to copy-construct from
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Reset(const ArrayType& InputArray) noexcept
    {
        Reset(InputArray.Data(), InputArray.Size());
    }

    /**
     * @brief: Resets the container by moving elements from another array to this one.
     * 
     * @param InputArray: Array to copy-construct elements from
     */
    FORCEINLINE void Reset(TArray&& InputArray) noexcept
    {
        MoveFrom(Forward<TArray>(InputArray));
    }

    /** 
     * Resets the container and copy-construct a new array from an initializer-list
     *
     * @param InList: Initializer-list to copy-construct elements from
     */
    FORCEINLINE void Reset(std::initializer_list<ElementType> InList) noexcept
    {
        Reset(GetInitializerListData(InList), GetInitializerListSize(InList));
    }

    /** 
     * Fill the container with the specified value 
     *
     * @param InputElement: Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ElementType* Elements = Data();
        FillRange(Elements, InputElement, Size());
    }

    /**
     * @brief: Resizes the container with a new size, and default constructs them
     * 
     * @param NewSize: The new size of the array
     */
    inline void Resize(SizeType NewSize) noexcept
    {
        if (NewSize > ArraySize)
        {
            if (NewSize >= ArrayCapacity)
            {
                ReserveStorage(NewSize);
            }

            // NewSize is always larger than array-size...
            SizeType NumElementsToConstruct = NewSize - ArraySize;
            ElementType* LastElementPtr = Data() + ArraySize;

            // ...However, assert just in case
            Assert(NumElementsToConstruct > 0);

            DefaultConstructRange<ElementType>(LastElementPtr, NumElementsToConstruct);
            ArraySize = NewSize;
        }
        else if (NewSize < ArraySize)
        {
            SizeType NumElementsToDestruct = ArraySize - NewSize;
            Assert(NumElementsToDestruct > 0);

            InternalPopRange(NumElementsToDestruct);
        }
    }

    /**
     * @brief: Resizes the container with a new size, and constructs them with value 
     * 
     * @param NewSize: The new size of the array
     * @param Elements: Element to copy into all positions of the array
     */
    inline void Resize(SizeType NewSize, const ElementType& Element) noexcept
    {
        if (NewSize > ArraySize)
        {
            if (NewSize >= ArrayCapacity)
            {
                ReserveStorage(NewSize);
            }

            // NewSize is always larger than arraysize...
            SizeType NumElementsToConstruct = NewSize - ArraySize;
            ElementType* LastElementPtr = Data() + ArraySize;

            // ...However, assert just in case
            Assert(NumElementsToConstruct > 0);

            ConstructRangeFrom<ElementType>(LastElementPtr, NumElementsToConstruct, Element);
            ArraySize = NewSize;
        }
        else if (NewSize < ArraySize)
        {
            SizeType NumElementsToDestruct = ArraySize - NewSize;
            Assert(NumElementsToDestruct > 0);

            InternalPopRange(NumElementsToDestruct);
        }
    }

    /**
     * @brief: Reallocate the array to a new capacity
     * 
     * @param NewCapacity: The new capacity of the allocated array
     */
    inline void Reserve(SizeType NewCapacity) noexcept
    {
        if (NewCapacity != ArrayCapacity)
        {
            if (NewCapacity < ArraySize)
            {
                DestructRange<ElementType>(Data() + NewCapacity, ArraySize - NewCapacity);
                ArraySize = NewCapacity;
            }

            ReserveStorage(NewCapacity);
        }
    }

    /** 
     * Constructs a new element at the end of the array 
     *
     * @param Args: Arguments for the constructor of the element
     * @return: Returns a reference to the newly created element
     */
    template<typename... ArgTypes>
    inline ElementType& Emplace(ArgTypes&&... Args) noexcept
    {
        GrowIfNeeded();

        new(Data() + (ArraySize++)) ElementType(Forward<ArgTypes>(Args)...);
        return LastElement();
    }

    /**
     * @brief: Inserts a new element at the end of the array
     * 
     * @param Element: Element to insert into the array by copy
     * @return: Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Push(const ElementType& Element) noexcept
    {
        return Emplace(Element);
    }

    /**
     * @brief: Inserts a new element at the end of the array
     * 
     * @param Element: Element to insert into the array by move
     * @return: Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Push(ElementType&& Element) noexcept
    {
        return Emplace(Forward<ElementType>(Element));
    }

    /**
     * @brief: Constructs a new element at a specific position in the array
     *
     * @param Position: Position of the new element
     * @param Args: Arguments for the constructor of the element
     */
    template<typename... ArgTypes>
    inline void EmplaceAt(SizeType Position, ArgTypes&&... Args) noexcept
    {
        Assert(Position <= ArraySize);

        if (Position == ArraySize)
        {
            Emplace(Forward<ArgTypes>(Args)...);
        }
        else
        {
            ReserveForInsertion(Position, 1);
            new(Data() + Position) ElementType(Forward<ArgTypes>(Args)...);

            ArraySize++;
        }
    }

    /**
     * @brief: Constructs a new element at a specific position in the array
     *
     * @param Position: Iterator pointing to the position of the new element
     * @param Args: Arguments for the constructor of the element
     */
    template<typename... ArgTypes>
    FORCEINLINE void EmplaceAt(ConstIteratorType Position, ArgTypes&&... Args) noexcept
    {
        EmplaceAt(Position.GetIndex(), Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief: Insert a new element at a specific position in the array
     *
     * @param Position: Position of the new element
     * @param Element: Element to copy into the position
     */
    FORCEINLINE void Insert(SizeType Position, const ElementType& Element) noexcept
    {
        EmplaceAt(Position, Element);
    }

    /**
     * @brief: Insert a new element at a specific position in the array
     *
     * @param Position: Iterator pointing to the position of the new element
     * @param Element: Element to copy into the position
     */
    FORCEINLINE void Insert(ConstIteratorType Position, const ElementType& Element) noexcept
    {
        EmplaceAt(Position.GetIndex(), Element);
    }

    /**
     * @brief: Insert a new element at the a specific position in the array by moving
     *
     * @param Position: Position of the new element
     * @param Element: Element to move into the position
     */
    FORCEINLINE void Insert(SizeType Position, ElementType&& Element) noexcept
    {
        EmplaceAt(Position, Forward<ElementType>(Element));
    }

    /**
     * @brief: Insert a new element at the a specific position in the array by moving
     *
     * @param Position: Iterator pointing to the position of the new element
     * @param Element: Element to move into the position
     */
    FORCEINLINE void Insert(ConstIteratorType Position, ElementType&& Element) noexcept
    {
        EmplaceAt(Position.GetIndex(), Forward<ElementType>(Element));
    }

    /**
     * @brief: Insert an array at a specific position in the array
     *
     * @param Position: Position of the new element
     * @param InputArray: Array to copy into the array
     * @param NumElements: Number of elements in the input-array
     */
    inline void Insert(SizeType Position, const ElementType* InputArray, SizeType NumElements) noexcept
    {
        Assert(Position <= ArraySize);
        Assert(InputArray != nullptr);

        if (Position == ArraySize)
        {
            Append(InputArray, NumElements);
        }
        else
        {
            ReserveForInsertion(Position, NumElements);
            CopyConstructRange<ElementType>(Data() + Position, InputArray, NumElements);

            ArraySize += NumElements;
        }
    }

    /**
     * @brief: Insert an array at a specific position in the array
     *
     * @param Position: Iterator pointing to the position of the new element
     * @param InputArray: Array to copy into the array
     * @param NumElements: Number of elements in the input-array
     */
    FORCEINLINE void Insert(ConstIteratorType Position, const ElementType* InputArray, SizeType NumElements) noexcept
    {
        Insert(Position.GetIndex(), InputArray, NumElements);
    }

    /**
     * @brief: Insert elements from a initializer list at a specific position in the array
     *
     * @param Position: Position of the new element
     * @param InList: Initializer list to insert into the array
     */
    FORCEINLINE void Insert(SizeType Position, std::initializer_list<ElementType> InList) noexcept
    {
        Insert(Position, GetInitializerListData(InList), GetInitializerListSize(InList));
    }

    /**
     * @brief: Insert elements from a initializer list at a specific position in the array
     *
     * @param Position: Iterator pointing to the position of the new element
     * @param InList: Initializer list to insert into the array
     */
    FORCEINLINE void Insert(ConstIteratorType Position, std::initializer_list<ElementType> InList) noexcept
    {
        Insert(Position.GetIndex(), GetInitializerListData(InList), GetInitializerListSize(InList));
    }

    /**
      * Insert elements from another array at a specific position in the array, which
      * can be of another array-type.
      *
      * @param Position: Position of the new element
      * @param InArray: Array to copy elements from
      */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Insert(SizeType Position, const ArrayType& InArray) noexcept
    {
        Insert(Position, InArray.Data(), InArray.Size());
    }

    /**
      * Insert elements from another array at a specific position in the array, which
      * can be of another array-type.
      *
      * @param Position: Iterator pointing to the position of the new element
      * @param InArray: Array to copy elements from
      */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Insert(ConstIteratorType Position, const ArrayType& InArray) noexcept
    {
        Insert(Position.GetIndex(), InArray.Data(), InArray.Size());
    }

    /**
     * @brief: Insert an array from a raw-pointer at the end of the array
     * 
     * @param InputArray: Array to copy elements from
     * @param NumElements: Number of elements in the input-array
     */
    inline void Append(const ElementType* InputArray, SizeType NumElements) noexcept
    {
        if (NumElements > 0)
        {
            Assert(InputArray != nullptr);

            const SizeType NewSize = ArraySize + NumElements;
            GrowIfNeeded(NewSize);

            CopyConstructRange<ElementType>(Data() + ArraySize, InputArray, NumElements);
            ArraySize = NewSize;
        }
    }

    /**
     * @brief: Insert another array at the end of the array, which can be of another array-type
     *
     * @param Other: Array to copy elements from
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Append(const ArrayType& Other) noexcept
    {
        Append(Other.Data(), Other.Size());
    }

    /**
     * @brief: Insert an initializer-list at the end of the array, which can be of another array-type
     *
     * @param InList: Initializer-list to copy elements from
     */
    FORCEINLINE void Append(std::initializer_list<ElementType> InList) noexcept
    {
        Append(GetInitializerListData(InList), GetInitializerListSize(InList));
    }

    /**
     * @brief: Create a number of uninitialized elements at the end of the array
     * 
     * @param NewSize: Number of elements to append
     */
    FORCEINLINE void AppendUninitialized(SizeType NumElements) noexcept
    {
        GrowIfNeeded(NumElements);
        ArraySize += NumElements;
    }

    /**
     * @brief: Remove and destroy a number of elements from the back
     *
     * @param NumElements: Number of elements destroy from the end
     */
    FORCEINLINE void PopRange(SizeType NumElements) noexcept
    {
        if (!IsEmpty())
        {
            InternalPopRange(NumElements);
        }
    }

    /** 
     * Remove the last element 
     */
    FORCEINLINE void Pop() noexcept
    {
        PopRange(1);
    }

    /**
     * @brief: Remove a range of elements starting at position
     * 
     * @param Position: Position of the array to start remove elements from
     * @param NumElements: Number of elements to remove
     */
    inline void RemoveRangeAt(SizeType Position, SizeType NumElements) noexcept
    {
        Assert(Position + NumElements <= ArraySize);

        if (Position + NumElements == ArraySize)
        {
            InternalPopRange(NumElements);
        }
        else
        {
            DestructRange<ElementType>(Data() + Position, NumElements);
            RelocateRange<ElementType>(Data() + Position, Data() + Position + NumElements, ArraySize - (Position + NumElements));
            ArraySize -= NumElements;
        }
    }

    /**
     * @brief: Removes the element at the position 
     * 
     * @param Position: Position of element to remove
     */
    FORCEINLINE void RemoveAt(SizeType Position) noexcept
    {
        RemoveRangeAt(Position, 1);
    }

    /**
     * @brief: Removes the element at the position
     *
     * @param Position: Iterator pointing to the position of element to remove
     * @return: Returns the iterator to the element that follows the removed element
     */
    FORCEINLINE IteratorType RemoveAt(IteratorType Iterator) noexcept
    {
        Assert(Iterator.IsFrom(*this));

        RemoveAt(Iterator.GetIndex());
        return Iterator;
    }

    /**
     * @brief: Removes the element at the position
     *
     * @param Position: Iterator pointing to the position of element to remove
     * @return: Returns the iterator to the element that follow the removed element
     */
    FORCEINLINE ConstIteratorType RemoveAt(ConstIteratorType Iterator) noexcept
    {
        Assert(Iterator.IsFrom(*this));

        RemoveAt(Iterator.GetIndex());
        return Iterator;
    }

    /**
     * @brief: Search the array and remove the first instance of the element from the array if it is found. 
     * 
     * @param Element: Element to remove
     */
    FORCEINLINE void Remove(const ElementType& Element) noexcept
    {
        for (IteratorType Iterator = StartIterator(); Iterator != EndIterator(); )
        {
            if (Element == *Iterator)
            {
                Iterator = RemoveAt(Iterator);
                break;
            }
            else
            {
                ++Iterator;
            }
        }
    }

    /**
     * @brief: Search the array and remove the all instances of the element from the array if it is found.
     *
     * @param Element: Element to remove
     */
    FORCEINLINE void RemoveAllOf(const ElementType& Element) noexcept
    {
        for (IteratorType Iterator = StartIterator(); Iterator != EndIterator(); )
        {
            if (Element == *Iterator)
            {
                Iterator = RemoveAt(Iterator);
            }
            else
            {
                ++Iterator;
            }
        }
    }

    /**
     * @brief: Check if an element exists in the array
     * 
     * @param Element: Element to check for
     * @return: Returns true if the element is found in the array and false if not
     */
    FORCEINLINE bool Contains(const ElementType& Element) const noexcept
    {
        for (ConstIteratorType Iterator = StartIterator(); Iterator != EndIterator(); ++Iterator)
        {
            if (Element == *Iterator)
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief: Check if an element that satisfies the conditions of a comparator exists in the array
     * 
     * @param Comparator: Callable that compares an element in the array against some condition
     * @return: Returns true if the comparator returned true for one element
     */
    template<class ComparatorType>
    FORCEINLINE bool Contains(ComparatorType Comparator) const noexcept
    {
        for (ConstIteratorType Iterator = StartIterator(); Iterator != EndIterator(); ++Iterator)
        {
            if (Comparator(*Iterator))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief: Check if an element that satisfies the conditions of a comparator
     *
     * @param Element: Element to compare with
     * @param Comparator: Callable that compares an element in the array against some condition
     * @return: Returns true if there is an element that satisfies the conditions of a comparator
     */
    template<class ComparatorType>
    FORCEINLINE SizeType Contains(const ElementType& Element, ComparatorType Comparator) const noexcept
    {
        for (ConstIteratorType Iterator = StartIterator(); Iterator != EndIterator(); ++Iterator)
        {
            if (Comparator(Element, *Iterator))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief: Returns the index of an element if it is present in the array, or -1 if it is not found
     * 
     * @param Element: Element to search for
     * @return: The index of the element if found or -1 if not
     */
    FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
    {
        for (ConstIteratorType Iterator = StartIterator(); Iterator != EndIterator(); ++Iterator)
        {
            if (Element == *Iterator)
            {
                return Iterator.GetIndex();
            }
        }

        return SizeType(-1);
    }

    /**
     * @brief: Returns the index of the element that satisfy the conditions of a comparator 
     *
     * @param Comparator: Callable that compares an element in the array against some condition
     * @return: The index of the element if found or -1 if not
     */
    template<class ComparatorType>
    FORCEINLINE SizeType Find(ComparatorType Comparator) const noexcept
    {
        for (ConstIteratorType Iterator = StartIterator(); Iterator != EndIterator(); ++Iterator)
        {
            if (Comparator(*Iterator))
            {
                return Iterator.GetIndex();
            }
        }

        return SizeType(-1);
    }

    /**
     * @brief: Returns the index of the element that satisfy the conditions of a comparator
     *
     * @param Element: Element to compare with
     * @param Comparator: Callable that compares an element in the array against some condition
     * @return: The index of the element if found or -1 if not
     */
    template<class ComparatorType>
    FORCEINLINE SizeType Find(const ElementType& Element, ComparatorType Comparator) const noexcept
    {
        for (ConstIteratorType Iterator = StartIterator(); Iterator != EndIterator(); ++Iterator)
        {
            if (Comparator(Element, *Iterator))
            {
                return Iterator.GetIndex();
            }
        }

        return SizeType(-1);
    }

    /**
     * @brief: Perform some function on each element in the array
     * 
     * @param Functor: Callable that takes one element and perform some operation on it
     */
    template<class FunctorType>
    FORCEINLINE void Foreach(FunctorType Functor)
    {
        for (IteratorType Iterator = StartIterator(); Iterator != EndIterator(); ++Iterator)
        {
            Functor(*Iterator);
        }
    }

    /**
     * @brief: Swap the contents of this array with another
     * 
     * @param Other: The other array to swap with
     */
    FORCEINLINE void Swap(TArray& Other) noexcept
    {
        TArray TempArray(Move(*this));
        MoveFrom(Move(Other));
        Other.MoveFrom(Move(TempArray));
    }

    /**
     * @brief: Shrink the allocation to perfectly fit with the size of the array
     */
    FORCEINLINE void ShrinkToFit() noexcept
    {
        Reserve(ArraySize);
    }

    /**
     * @brief: Check if the container contains any elements
     * 
     * @return: Returns true if the array is empty or false if it contains elements
     */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ArraySize == 0);
    }

    /**
     * @brief: Retrieve the first element of the array
     * 
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /**
     * @brief: Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[ArraySize - 1];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[ArraySize - 1];
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE ElementType* Data() noexcept
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const ElementType* Data() const noexcept
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the array
     *
     * @return: Returns a the index to the last element of the array
     */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (ArraySize > 0) ? (ArraySize - 1) : 0;
    }

    /**
     * @brief: Returns the size of the container
     * 
     * @return: The current size of the container
     */
    FORCEINLINE SizeType Size() const noexcept
    {
        return ArraySize;
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
     * @brief: Returns the capacity of the container
     *
     * @return: The current capacity of the container
     */
    FORCEINLINE SizeType Capacity() const noexcept
    {
        return ArrayCapacity;
    }

    /**
     * @brief: Returns the capacity of the container in bytes
     *
     * @return: The current capacity of the container in bytes
     */
    FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return Capacity() * sizeof(ElementType);
    }

    /**
     * @brief: Retrieve a element at a certain index of the array
     * 
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE ElementType& At(SizeType Index) noexcept
    {
        Assert(Index < ArraySize);
        return Data()[Index];
    }

    /**
     * @brief: Retrieve a element at a certain index of the array
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const ElementType& At(SizeType Index) const noexcept
    {
        Assert(Index < ArraySize);
        return Data()[Index];
    }

    /**
     * @brief: Create an array-view of the array
     *
     * @return: A new array-view pointing this array's data
     */
    FORCEINLINE TArrayView<ElementType> CreateView() noexcept
    {
        return TArrayView<ElementType>(Data(), Size());
    }

    /**
     * @brief: Create an array-view of the array
     *
     * @return: A new array-view pointing this array's data
     */
    FORCEINLINE TArrayView<const ElementType> CreateView() const noexcept
    {
        return TArrayView<const ElementType>(Data(), Size());
    }

    /**
     * @brief: Create an array-view of the array
     * 
     * @param Offset: Offset into the array
     * @param NumElements: Number of elements to include in the view
     * @return: A new array-view pointing to the specified elements
     */
    FORCEINLINE TArrayView<ElementType> CreateView(SizeType Offset, SizeType NumElements) noexcept
    {
        Assert((NumElements < ArraySize) && (Offset + NumElements < ArraySize));
        return TArrayView<ElementType>(Data() + Offset, NumElements);
    }

    /**
     * @brief: Create an array-view of the array
     *
     * @param Offset: Offset into the array
     * @param NumElements: Number of elements to include in the view
     * @return: A new array-view pointing to the specified elements
     */
    FORCEINLINE TArrayView<const ElementType> CreateView(SizeType Offset, SizeType NumElements) const noexcept
    {
        Assert((NumElements < ArraySize) && (Offset + NumElements < ArraySize));
        return TArrayView<const ElementType>(Data() + Offset, NumElements);
    }

    /**
     * @brief: Create a heap of the array 
     */
    inline void Heapify() noexcept
    {
        const SizeType StartIndex = (ArraySize / 2) - 1;
        for (SizeType i = StartIndex; i >= 0; i--)
        {
            Heapify(ArraySize, i);
        }
    }

    /**
     * @brief: Retrieve the top of the heap. The same as the first element.
     * 
     * @return: A reference to the element at the top of the heap
     */
    FORCEINLINE ElementType& HeapTop() noexcept
    {
        return Data()[0];
    }

    /**
     * @brief: Retrieve the top of the heap. The same as the first element.
     *
     * @return: A reference to the element at the top of the heap
     */
    FORCEINLINE const ElementType& HeapTop() const noexcept
    {
        return Data()[0];
    }

    /**
     * @brief: Inserts a new element at the top of the heap
     * 
     * @param Element: Element to copy to the top of the heap
     */
    FORCEINLINE void HeapPush(const ElementType& Element) noexcept
    {
        Insert(0, Element);
        Heapify(ArraySize, 0);
    }

    /**
     * @brief: Inserts a new element at the top of the heap
     *
     * @param Element: Element to move to the top of the heap
     */
    FORCEINLINE void HeapPush(ElementType&& Element) noexcept
    {
        Insert(0, Forward<ElementType>(Element));
        Heapify(ArraySize, 0);
    }

    /**
     * @brief: Remove the top of the heap and retrieve the element on top
     * 
     * @param OutElement: Reference that the top element will be copied to
     */
    FORCEINLINE void HeapPop(ElementType& OutElement) noexcept
    {
        OutElement = HeapTop();
        HeapPop();
    }

    /**
     * @brief: Remove the top of the heap
     */
    FORCEINLINE void HeapPop() noexcept
    {
        RemoveAt(0);
        Heapify();
    }

    /**
     * @brief: Performs heap sort on the array (assuming the operator> exists for the elements)
     */
    FORCEINLINE void HeapSort()
    {
        Heapify();

        for (SizeType Index = ArraySize - 1; Index > 0; Index--)
        {
            ::Swap<ElementType>(At(0), At(Index));
            Heapify(Index, 0);
        }
    }

public:

    /**
     * @brief: Copy-assignment operator
     * 
     * @param RHS: Array to copy
     * @return: A reference to this container
     */
    FORCEINLINE TArray& operator=(const TArray& RHS) noexcept
    {
        Reset(RHS);
        return *this;
    }


    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: Array to move
     * @return: A reference to this container
     */
    FORCEINLINE TArray& operator=(TArray&& RHS) noexcept
    {
        MoveFrom(Forward<TArray>(RHS));
        return *this;
    }

    /**
     * @brief: Assignment-operator that takes a initializer-list
     * 
     * @param RHS: A initializer list to replace the current contents with
     * @return: A reference to this container
     */
    FORCEINLINE TArray& operator=(std::initializer_list<ElementType> RHS) noexcept
    {
        Reset(RHS);
        return *this;
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

private:

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Internal functions

    FORCEINLINE void InitUnitialized(SizeType NumElements)
    {
        if (ArrayCapacity < NumElements)
        {
            Allocator.Realloc(ArrayCapacity, NumElements);
            ArrayCapacity = NumElements;
        }

        ArraySize = NumElements;
    }

    FORCEINLINE void EmptyConstruct(SizeType NumElements)
    {
        InitUnitialized(NumElements);
        DefaultConstructRange<ElementType>(Data(), NumElements);
    }

    FORCEINLINE void EmptyConstructFrom(SizeType NumElements, const ElementType& Element)
    {
        InitUnitialized(NumElements);
        ConstructRangeFrom<ElementType>(Data(), NumElements, Element);
    }

    FORCEINLINE void CopyConstructFrom(const ElementType* From, SizeType NumElements)
    {
        InitUnitialized(NumElements);
        CopyConstructRange<ElementType>(Data(), From, NumElements);
    }

    FORCEINLINE void MoveFrom(TArray&& FromArray)
    {
        if (FromArray.Data() != Data())
        {
            // Since the memory remains the same we should not need to use move-assignment or constructor. However, still need to call destructors
            DestructRange<ElementType>(Data(), Size());
            Allocator.MoveFrom(Move(FromArray.Allocator));

            ArraySize               = FromArray.ArraySize;
            ArrayCapacity           = FromArray.ArrayCapacity;
            FromArray.ArraySize     = 0;
            FromArray.ArrayCapacity = 0;
        }
    }

    template<typename U = T>
    FORCEINLINE typename TEnableIf<TIsReallocatable<U>::Value>::Type ReserveStorage(const SizeType NewCapacity) noexcept
    {
        Allocator.Realloc(ArrayCapacity, NewCapacity);
        ArrayCapacity = NewCapacity;
    }

    template<typename U = T>
    FORCEINLINE typename TEnableIf<TNot<TIsReallocatable<U>>::Value>::Type ReserveStorage(const SizeType NewCapacity) noexcept
    {
        if (ArrayCapacity)
        {
            /* For non-trivial objects a new allocator is necessary in order to correctly reallocate objects. This in case
               objects has references to themselves or "child-objects" that references these objects. */
            AllocatorType NewAllocator;
            NewAllocator.Realloc(ArrayCapacity, NewCapacity);

            RelocateRange<ElementType>(NewAllocator.GetAllocation(), Data(), ArraySize);

            Allocator.MoveFrom(Move(NewAllocator));
        }
        else
        {
            Allocator.Realloc(ArrayCapacity, NewCapacity);
        }

        ArrayCapacity = NewCapacity;
    }

    FORCEINLINE void ReserveForInsertion(const SizeType Position, const SizeType ElementNumElements) noexcept
    {
        const SizeType NewSize = ArraySize + ElementNumElements;
        GrowIfNeeded(NewSize);

        RelocateRange<ElementType>(Data() + Position + ElementNumElements, Data() + Position, ArraySize - Position);
    }

    FORCEINLINE void InternalPopRange(SizeType NumElements) noexcept
    {
        ArraySize = ArraySize - NumElements;
        DestructRange<ElementType>(Data() + ArraySize, NumElements);
    }

    FORCEINLINE void GrowIfNeeded() noexcept
    {
        GrowIfNeeded(ArraySize + 1);
    }

    FORCEINLINE void GrowIfNeeded(SizeType NewSize) noexcept
    {
        if (NewSize > ArrayCapacity)
        {
            const SizeType NewCapacity = GetGrowCapacity(NewSize, ArrayCapacity);
            ReserveStorage(NewCapacity);
        }
    }

    static FORCEINLINE SizeType LeftIndex(SizeType Index)
    {
        return (2 * Index + 1);
    }

    static FORCEINLINE SizeType RightIndex(SizeType Index)
    {
        return (2 * Index + 2);
    }

    // TODO: Better to have top in back? Better to do recursive?
    FORCEINLINE void Heapify(SizeType Size, SizeType Index) noexcept
    {
        SizeType StartIndex = Index;
        SizeType Largest    = Index;

        while (true)
        {
            const SizeType Left = LeftIndex(StartIndex);
            const SizeType Right = RightIndex(StartIndex);

            if (Left < Size && At(Left) > At(Largest))
            {
                Largest = Left;
            }

            if (Right < Size && At(Right) > At(Largest))
            {
                Largest = Right;
            }

            if (Largest != StartIndex)
            {
                ::Swap<ElementType>(At(StartIndex), At(Largest));
                StartIndex = Largest;
            }
            else
            {
                break;
            }
        }
    }

    // Calculate how much the array should grow, will always be at least one
    static FORCEINLINE SizeType GetGrowCapacity(SizeType NewSize, SizeType CurrentCapacity) noexcept
    {
        SizeType NewCapacity = NewSize + SizeType(float(CurrentCapacity) * 0.5f);
        return (NewCapacity >= 0) ? NewCapacity : 1;
    }

    AllocatorType Allocator;

    SizeType ArraySize;
    SizeType ArrayCapacity;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Enable TArrayType

template<typename T, typename AllocatorType>
struct TIsTArrayType<TArray<T, AllocatorType>>
{
    enum
    {
        Value = true
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Allocate and copy the contents into a unique-ptr

template<typename T, typename AllocatorType>
inline TUniquePtr<T[]> MakeUniquePtr(const TArray<T, AllocatorType>& Array) noexcept
{
    T* Memory = CMemory::Malloc<T>(Array.Size());
    CopyConstructRange<T>(Memory, Array.Data(), Array.Size());
    return TUniquePtr<T[]>(Memory);
}
