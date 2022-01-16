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

#include <initializer_list>

/* Dynamic Array similar to std::vector */
template<typename T, typename AllocatorType = TDefaultArrayAllocator<T>>
class TArray
{
public:

    using ElementType = T;
    using SizeType = int32;

    /* Iterators */
    typedef TArrayIterator<TArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArray, const ElementType> ReverseConstIteratorType;

    /* Empty default constructor */
    FORCEINLINE TArray() noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
    }

    /* Default creates a certain number of elements */
    FORCEINLINE explicit TArray(SizeType InSize) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        EmptyConstruct(InSize);
    }

    /* Allocates the specified amount of elements, and initializes them to the same value */
    FORCEINLINE explicit TArray(SizeType InSize, const ElementType& Element) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        EmptyConstructFrom(InSize, Element);
    }

    /* Creates an array from a raw pointer array */
    FORCEINLINE explicit TArray(const ElementType* InputArray, SizeType Count) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(InputArray, Count);
    }

    /* Creates an array from an std::initializer_list */
    FORCEINLINE TArray(std::initializer_list<ElementType> InitList) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(InitList.begin(), static_cast<SizeType>(InitList.size()));
    }

    /* Copy-constructs an array from another array */
    FORCEINLINE TArray(const TArray& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(Other.Data(), Other.Size());
    }

    /* Copy-constructs an array from another array */
    template<typename ArrayType, typename = typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type>
    FORCEINLINE explicit TArray(const ArrayType& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(Other.Data(), Other.Size());
    }

    /* Move-constructs an array from another array */
    FORCEINLINE TArray(TArray&& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        MoveFrom(Forward<TArray>(Other));
    }

    /* Destructor deleting the array */
    FORCEINLINE ~TArray()
    {
        Empty();
    }

    /* Empties the array and deallocates the memory */
    FORCEINLINE void Empty() noexcept
    {
        if (ArraySize)
        {
            DestructRange<ElementType>(Data(), ArraySize);
            ArraySize = 0;
        }

        Allocator.Free();
        ArrayCapacity = 0;
    }

    /* Clears the container, but does not deallocate the memory */
    FORCEINLINE void Clear() noexcept
    {
        DestructRange<ElementType>(Data(), ArraySize);
        ArraySize = 0;
    }

    /* Resets the container, but does not deallocate the memory */
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

    /* Resets the container, but does not deallocate the memory */
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

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset(const ElementType* InputArray, SizeType Count) noexcept
    {
        Assert((InputArray != nullptr) && (Count > 0));

        if (InputArray != Data())
        {
            DestructRange<ElementType>(Data(), ArraySize);

            if (Count)
            {
                CopyConstructFrom(InputArray, Count);
            }
            else
            {
                ArraySize = 0;
            }
        }
    }

    /* Resets the container, but does not deallocate the memory */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Reset(const ArrayType& InputArray) noexcept
    {
        Reset(InputArray.Data(), InputArray.Size());
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset(TArray&& InputArray) noexcept
    {
        MoveFrom(Forward<TArray>(InputArray));
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset(std::initializer_list<ElementType> InitList) noexcept
    {
        Reset(InitList.begin(), static_cast<SizeType>(InitList.size()));
    }

    /* Fills the container with the specified value */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        for (ElementType& Element : *this)
        {
            Element = InputElement;
        }
    }

    /* Resizes the container with a new size, and default constructs them*/
    inline void Resize(SizeType NewSize) noexcept
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

    /* Resizes the container with a new size, and constructs them with value */
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

    /* Resizes the allocation */
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

    /* Constructs a new element at the end */
    template<typename... ArgTypes>
    inline ElementType& Emplace(ArgTypes&&... Args) noexcept
    {
        GrowIfNeeded();

        new(Data() + (ArraySize++)) ElementType(Forward<ArgTypes>(Args)...);
        return LastElement();
    }

    /* Inserts a new element at the end */
    FORCEINLINE ElementType& Push(const ElementType& Element) noexcept
    {
        return Emplace(Element);
    }

    /* Inserts a new element at the end */
    FORCEINLINE ElementType& Push(ElementType&& Element) noexcept
    {
        return Emplace(Forward<ElementType>(Element));
    }

    /* Constructs a new element at specified position */
    template<typename... ArgTypes>
    inline void EmplaceAt(SizeType Position, ArgTypes&&... Args) noexcept
    {
        Assert(Position <= ArraySize);

        /* Simple path if position is last */
        if (Position == ArraySize)
        {
            Emplace(Forward<ArgTypes>(Args)...);
        }
        else
        {
            /* Make room within array and allocate if necessary */
            ReserveForInsertion(Position, 1);
            new(Data() + Position) ElementType(Forward<ArgTypes>(Args)...);

            /* Update size */
            ArraySize++;
        }
    }

    /* Inserts an element at the specified position */
    FORCEINLINE void Insert(SizeType Position, const ElementType& Element) noexcept
    {
        EmplaceAt(Position, Element);
    }

    /* Inserts an element at the specified position */
    FORCEINLINE void Insert(SizeType Position, ElementType&& Element) noexcept
    {
        EmplaceAt(Position, Forward<ElementType>(Element));
    }

    /* Insert an array into the container at the position */
    inline void Insert(SizeType Position, const ElementType* InputArray, SizeType Count) noexcept
    {
        Assert(Position <= ArraySize);
        Assert(InputArray != nullptr);

        /* Special case if at the end */
        if (Position == ArraySize)
        {
            Append(InputArray, Count);
        }
        else
        {
            /* Make room within array and allocate if necessary */
            ReserveForInsertion(Position, Count);
            CopyConstructRange<ElementType>(Data() + Position, InputArray, Count);

            /* Update size */
            ArraySize += Count;
        }
    }

    /* Insert an std::initializer_list into the container at the position */
    FORCEINLINE void Insert(SizeType Position, std::initializer_list<ElementType> InitList) noexcept
    {
        Insert(Position, InitList.begin(), static_cast<SizeType>(InitList.size()));
    }

    /* Insert an array into the container at the position */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Insert(SizeType Position, const ArrayType& InArray) noexcept
    {
        Insert(Position, InArray.Data(), InArray.Size());
    }

    /* Inserts an array at the end */
    inline void Append(const ElementType* InputArray, SizeType NumElements) noexcept
    {
        Assert(InputArray != nullptr);

        const SizeType NewSize = ArraySize + NumElements;
        GrowIfNeeded(NewSize);

        CopyConstructRange<ElementType>(Data() + ArraySize, InputArray, NumElements);
        ArraySize = NewSize;
    }

    /* Inserts an array at the end */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Append(const ArrayType& Other) noexcept
    {
        Append(Other.Data(), Other.Size());
    }

    /* Inserts an array at the end */
    FORCEINLINE void Append(std::initializer_list<ElementType> InitList) noexcept
    {
        Append(InitList.begin(), static_cast<SizeType>(InitList.size()));
    }

    /* Append unintialized, this modifies the size of the array and does not call any constructor */
    FORCEINLINE void AppendUninitialized(SizeType NewSize) noexcept
    {
        GrowIfNeeded(NewSize);
        ArraySize = NewSize;
    }

    /* Removes a number of elments from the back */
    FORCEINLINE void PopRange(SizeType Count) noexcept
    {
        if (!IsEmpty())
        {
            InternalPopRange(Count);
        }
    }

    /* Removes the last element */
    FORCEINLINE void Pop() noexcept
    {
        PopRange(1);
    }

    /* Remove a range starting at position and containg count number of elements */
    inline void RemoveRangeAt(SizeType Position, SizeType Count) noexcept
    {
        Assert(Position + Count <= ArraySize);

        if (Position + Count == ArraySize)
        {
            InternalPopRange(Count);
        }
        else
        {
            DestructRange<ElementType>(Data() + Position, Count);
            RelocateRange<ElementType>(Data() + Position, Data() + Position + Count, ArraySize - (Position + Count));
            ArraySize -= Count;
        }
    }

    /* Removes the element at the position */
    FORCEINLINE void RemoveAt(SizeType Position) noexcept
    {
        RemoveRangeAt(Position, 1);
    }

    /* Removes the element pointed to by a iterator at the position */
    FORCEINLINE IteratorType RemoveAt(IteratorType Iterator) noexcept
    {
        Assert(Iterator.IsFrom(*this));
        RemoveAt(Iterator.GetIndex());
        return Iterator;
    }

    /* Removes the element pointed to by a iterator at the position */
    FORCEINLINE ConstIteratorType RemoveAt(ConstIteratorType Iterator) noexcept
    {
        Assert(Iterator.IsFrom(*this));
        RemoveAt(Iterator.GetIndex());
        return Iterator;
    }

    /* Compares the elements in the array and removes the element if found */
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

    /* Compares all the elements in the array and removes all instance of the element if found */
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

    /* Returns true if the element exists in the array */
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

    /* Returns true if the element exists in the array */
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

    /* Returns true if the element exists in the array */
    template<class ComparatorType>
    FORCEINLINE bool Contains(const ElementType& Element, ComparatorType Comparator) const noexcept
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

    /* Returns the index of the element if found, otherwise -1 */
    FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
    {
        const SizeType CurrentSize = Size();
        for (SizeType Index = 0; Index < CurrentSize; ++Index)
        {
            if (Element == At(Index))
            {
                return Index;
            }
        }

        return SizeType(-1);
    }

    /* Returns the index of the element if found, otherwise -1 */
    template<class ComparatorType>
    FORCEINLINE SizeType Find(ComparatorType Comparator) const noexcept
    {
        const SizeType CurrentSize = Size();
        for (SizeType Index = 0; Index < CurrentSize; ++Index)
        {
            if (Comparator(At(Index)))
            {
                return Index;
            }
        }

        return SizeType(-1);
    }

    /* Returns the index of the element if found, otherwise -1 */
    template<class ComparatorType>
    FORCEINLINE SizeType Find(const ElementType& Element, ComparatorType Comparator) const noexcept
    {
        const SizeType CurrentSize = Size();
        for (SizeType Index = 0; Index < CurrentSize; ++Index)
        {
            if (Comparator(Element, At(Index)))
            {
                return Index;
            }
        }

        return SizeType(-1);
    }

    /* For each element the functor or function is called */
    template<class FunctorType>
    FORCEINLINE void Foreach(FunctorType Functor)
    {
        const SizeType CurrentSize = Size();
        for (SizeType Index = 0; Index < CurrentSize; ++Index)
        {
            Functor(At(Index));
        }
    }

    /* Swaps container with another */
    FORCEINLINE void Swap(TArray& Other) noexcept
    {
        TArray TempArray(Move(*this));
        MoveFrom(Move(Other));
        Other.MoveFrom(Move(TempArray));
    }

    /* Shrinks the allocation to be the same as the size */
    FORCEINLINE void ShrinkToFit() noexcept
    {
        Reserve(ArraySize);
    }

    /* Checks if there are any elements */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ArraySize == 0);
    }

    /* Returns the first element of the container */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /* Returns the first element of the container */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /* Returns the last element of the container */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[ArraySize - 1];
    }

    /* Returns the last element of the container */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[ArraySize - 1];
    }

    /* Returns the data of the container */
    FORCEINLINE ElementType* Data() noexcept
    {
        return Allocator.GetAllocation();
    }

    /* Returns the data of the container */
    FORCEINLINE const ElementType* Data() const noexcept
    {
        return Allocator.GetAllocation();
    }

    /* Returns the last valid index the container */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (ArraySize > 0) ? (ArraySize - 1) : 0;
    }

    /* Returns the size of the container */
    FORCEINLINE SizeType Size() const noexcept
    {
        return ArraySize;
    }

    /* Returns the size of the container in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof(ElementType);
    }

    /* Returns the capacity of the container */
    FORCEINLINE SizeType Capacity() const noexcept
    {
        return ArrayCapacity;
    }

    /* Returns the capacity of the container in bytes */
    FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return Capacity() * sizeof(ElementType);
    }

    /* Returns the element at a certain index */
    FORCEINLINE ElementType& At(SizeType Index) noexcept
    {
        Assert(Index < ArraySize);
        return Data()[Index];
    }

    /* Returns the element at a certain index */
    FORCEINLINE const ElementType& At(SizeType Index) const noexcept
    {
        Assert(Index < ArraySize);
        return Data()[Index];
    }

    /* Create a view of array */
    FORCEINLINE TArrayView<ElementType> CreateView(SizeType Offset, SizeType Count) const noexcept
    {
        Assert((Count < ArraySize) && (Offset + Count < ArraySize));
        return TArrayView<ElementType>(Data() + Offset, Count);
    }

    /* Create a heap of the array */
    inline void Heapify() noexcept
    {
        const SizeType StartIndex = (ArraySize / 2) - 1;
        for (SizeType i = StartIndex; i >= 0; i--)
        {
            Heapify(ArraySize, i);
        }
    }

    /* Get the top of the heap */
    FORCEINLINE ElementType& HeapTop() noexcept
    {
        return Data()[0];
    }

    /* Get the top of the heap */
    FORCEINLINE const ElementType& HeapTop() const noexcept
    {
        return Data()[0];
    }

    /* Inserts a new element at the top of the heap */
    FORCEINLINE void HeapPush(const ElementType& Element) noexcept
    {
        Insert(0, Element);
        Heapify(ArraySize, 0);
    }

    /* Inserts a new element at the top of the heap */
    FORCEINLINE void HeapPush(ElementType&& Element) noexcept
    {
        Insert(0, Forward<ElementType>(Element));
        Heapify(ArraySize, 0);
    }

    /* Removes the top of the heap */
    FORCEINLINE void HeapPop(ElementType& OutElement) noexcept
    {
        OutElement = HeapTop();
        HeapPop();
    }

    /* Removes the top of the heap */
    FORCEINLINE void HeapPop() noexcept
    {
        RemoveAt(0);
        Heapify();
    }

    /* Performs heap sort on the array ( assuming > operator )*/
    FORCEINLINE void HeapSort()
    {
        Heapify();

        for (SizeType Index = ArraySize - 1; Index > 0; Index--)
        {
            ::Swap<ElementType>(At(0), At(Index));
            Heapify(Index, 0);
        }
    }

    /* Sets the container to another array by copying it */
    FORCEINLINE TArray& operator=(const TArray& Other) noexcept
    {
        Reset(Other);
        return *this;
    }

    /* Sets the container to another array by moving it */
    FORCEINLINE TArray& operator=(TArray&& Other) noexcept
    {
        MoveFrom(Forward<TArray>(Other));
        return *this;
    }

    /* Sets the container to a std::initializer_list */
    FORCEINLINE TArray& operator=(std::initializer_list<ElementType> InitList) noexcept
    {
        Reset(InitList);
        return *this;
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator==(const ArrayType& RHS) const noexcept
    {
        if (Size() != RHS.Size())
        {
            return false;
        }

        return CompareRange<ElementType>(Data(), RHS.Data(), Size());
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator!=(const ArrayType& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    /* Returns the elment at a certain index */
    FORCEINLINE ElementType& operator[](SizeType Index) noexcept
    {
        return At(Index);
    }

    /* Returns the elment at a certain index */
    FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        return At(Index);
    }

public:

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, Size());
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, Size());
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /* STL iterator functions - Enables Range-based for-loops */

    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:

    /* Internal functions */

    /* Adds uninitialized elements */
    FORCEINLINE void InitUnitialized(SizeType Count)
    {
        if (ArrayCapacity < Count)
        {
            Allocator.Realloc(ArrayCapacity, Count);
            ArrayCapacity = Count;
        }

        ArraySize = Count;
    }

    /* Default-construct array, assumes that the container is currently empty */
    FORCEINLINE void EmptyConstruct(SizeType Count)
    {
        InitUnitialized(Count);
        DefaultConstructRange<ElementType>(Data(), Count);
    }

    /* Construct array from Element, assumes that the container is currently empty */
    FORCEINLINE void EmptyConstructFrom(SizeType Count, const ElementType& Element)
    {
        InitUnitialized(Count);
        ConstructRangeFrom<ElementType>(Data(), Count, Element);
    }

    /* Copy from array, assumes that the container is currently empty */
    FORCEINLINE void CopyConstructFrom(const ElementType* From, SizeType Count)
    {
        InitUnitialized(Count);
        CopyConstructRange<ElementType>(Data(), From, Count);
    }

    /* Moves from one array to another */
    FORCEINLINE void MoveFrom(TArray&& FromArray)
    {
        if (FromArray.Data() != Data())
        {
            /* Since the memory remains the same we should not need to use move-assignment or constructor. However, still need to call our destructors */
            DestructRange<ElementType>(Data(), Size());
            Allocator.MoveFrom(Move(FromArray.Allocator));

            ArraySize = FromArray.ArraySize;
            ArrayCapacity = FromArray.ArrayCapacity;
            FromArray.ArraySize = 0;
            FromArray.ArrayCapacity = 0;
        }
    }

    /* Reserves storage */
    template<typename U = T>
    FORCEINLINE typename TEnableIf<TIsReallocatable<U>::Value>::Type ReserveStorage(const SizeType NewCapacity) noexcept
    {
        /* Simple Memory::Realloc for trivial elements */
        Allocator.Realloc(ArrayCapacity, NewCapacity);
        ArrayCapacity = NewCapacity;
    }

    /* Reserves storage */
    template<typename U = T>
    FORCEINLINE typename TEnableIf<TNot<TIsReallocatable<U>>::Value>::Type ReserveStorage(const SizeType NewCapacity) noexcept
    {
        if (ArrayCapacity)
        {
            /* For non-trivial objects a new allocator is necessary in order to correctly relocate objects. This in case
                objects has references to themselves or childobjects that references these objects. */
            AllocatorType NewAllocator;
            NewAllocator.Realloc(ArrayCapacity, NewCapacity);

            /* Relocate existing elements */
            RelocateRange<ElementType>(NewAllocator.GetAllocation(), Data(), ArraySize);

            /* Move allocator */
            Allocator.MoveFrom(Move(NewAllocator));
        }
        else
        {
            /* Allocate new memory */
            Allocator.Realloc(ArrayCapacity, NewCapacity);
        }

        ArrayCapacity = NewCapacity;
    }

    /* Reserves room and not necessarily new memory */
    FORCEINLINE void ReserveForInsertion(const SizeType Position, const SizeType ElementCount) noexcept
    {
        const SizeType NewSize = ArraySize + ElementCount;
        GrowIfNeeded(NewSize);

        /* Elements after new area */
        RelocateRange<ElementType>(Data() + Position + ElementCount, Data() + Position, ArraySize - Position);
    }

    /* Pop range internally, avoids branches when not needed */
    FORCEINLINE void InternalPopRange(SizeType Count) noexcept
    {
        ArraySize = ArraySize - Count;
        DestructRange<ElementType>(Data() + ArraySize, Count);
    }

    /* Grows the array if the size is too small */
    FORCEINLINE void GrowIfNeeded() noexcept
    {
        GrowIfNeeded(ArraySize + 1);
    }

    /* Grows the array if the requested size is too small */
    FORCEINLINE void GrowIfNeeded(SizeType NewSize) noexcept
    {
        if (NewSize > ArrayCapacity)
        {
            const SizeType NewCapacity = GetGrowCapacity(NewSize, ArrayCapacity);
            ReserveStorage(NewCapacity);
        }
    }

    // Get index of left child of node at index
    static FORCEINLINE SizeType LeftIndex(SizeType Index)
    {
        return (2 * Index + 1);
    }

    // Get index of right child of node at index
    static FORCEINLINE SizeType RightIndex(SizeType Index)
    {
        return (2 * Index + 2);
    }

    // TODO: Better to have top in back? Better to do recursive?

    /* Internal create a max-heap of the array */
    FORCEINLINE void Heapify(SizeType Size, SizeType Index) noexcept
    {
        SizeType StartIndex = Index;
        SizeType Largest = Index;

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

    /* Calculate how much the array should grow, will always be at least one */
    static FORCEINLINE SizeType GetGrowCapacity(SizeType NewSize, SizeType CurrentCapacity) noexcept
    {
        SizeType NewCapacity = NewSize + SizeType(float(CurrentCapacity) * 0.5f);
        return (NewCapacity >= 0) ? NewCapacity : 1;
    }

    /* Allocator contains the pointer */
    AllocatorType Allocator;
    SizeType      ArraySize;
    SizeType      ArrayCapacity;
};

/* Enable TArrayType */
template<typename T, typename AllocatorType>
struct TIsTArrayType<TArray<T, AllocatorType>>
{
    enum
    {
        Value = true
    };
};

/* Allocate and copy the contents into a uniqueptr */
template<typename T, typename AllocatorType>
inline TUniquePtr<T[]> MakeUniquePtr(const TArray<T, AllocatorType>& Array) noexcept
{
    T* Memory = CMemory::Malloc<T>(Array.Size());
    CopyConstructRange<T>(Memory, Array.Data(), Array.Size());
    return TUniquePtr<T[]>(Memory);
}
