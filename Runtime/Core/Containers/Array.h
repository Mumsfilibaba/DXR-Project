#pragma once
#include "Core/Containers/Allocators.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/Functional.h"
#include "Core/Math/Random.h"

template<typename ElementType, typename AllocatorType = TDefaultArrayAllocator<ElementType>>
class TArray
{
public:
    typedef int32 SIZETYPE;
    static_assert(TIsSigned<SIZETYPE>::Value, "TArray only supports a SIZETYPE that's signed");

    typedef TArrayIterator<TArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArray, const ElementType> ReverseConstIteratorType;

    inline static constexpr SIZETYPE INVALID_INDEX = SIZETYPE(~0);

public:

    /** @brief - Default constructor */
    TArray() = default;

    /** 
     * @brief        - Constructor that default creates a certain number of elements 
     * @param InSize - Number of elements to construct
     */
    FORCEINLINE explicit TArray(SIZETYPE InSize)
    {
        InitializeEmpty(InSize);
    }

    /**
     * @brief         - Constructor that Allocates the specified amount of elements, and initializes them to the same value 
     * @param InSize  - Number of elements to construct
     * @param Element - Element to copy into all positions of the array
     */
    FORCEINLINE TArray(SIZETYPE InSize, const ElementType& Element)
    {
        Initialize(InSize, Element);
    }

    /**
     * @brief               - Constructor that creates an array from a raw pointer array 
     * @param InElements    - Pointer to the start of the array to copy from
     * @param InNumElements - Number of elements in 'InputArray', which also is the resulting size of the constructed array
     * @param InSlack       - Extra number of elements to allocate space for
     */
    FORCEINLINE TArray(const ElementType* InElements, SIZETYPE InNumElements, SIZETYPE InSlack = 0)
    {
        InitializeByCopy(InElements, InNumElements, InSlack);
    }

    /** 
     * @brief       - Copy-constructor
     * @param Other - Array to copy from
     */
    FORCEINLINE TArray(const TArray& Other)
    {
        InitializeByCopy(Other.Data(), Other.Size(), 0);
    }

    /** 
     * @brief         - Copy-constructor with additional slack
     * @param Other   - Array to copy from
     * @param InSlack - Extra number of elements to allocate space for
     */
    FORCEINLINE TArray(const TArray& Other, SIZETYPE InSlack)
    {
        InitializeByCopy(Other.Data(), Other.Size(), InSlack);
    }

    /**
     * @brief       - Copy-constructor from another type of array
     * @param Other - Array to copy from
     */
    template<typename ArrayType>
    FORCEINLINE explicit TArray(const ArrayType& Other) requires(TIsTArrayType<ArrayType>::Value)
    {
        InitializeByCopy(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other), 0);
    }

    /** 
     * @brief       - Move-constructor 
     * @param Other - Array to move elements from
     */
    FORCEINLINE TArray(TArray&& Other)
    {
        InitializeByMove(Forward<TArray>(Other));
    }

    /** 
     * @brief          - Constructor that creates an array from an std::initializer_list
     * @param InitList - Initializer list containing all elements to construct the array from
     */
    FORCEINLINE TArray(std::initializer_list<ElementType> InitList)
    {
        InitializeByCopy(FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList), 0);
    }

    /** 
     * @brief - Destructor
     */
    FORCEINLINE ~TArray()
    {
        Clear(true);
    }

    /** 
     * @brief              - Clear all elements of the container, but does not deallocate the memory
     * @param bRemoveSlack - Should the memory be deallocated or not
     */
    void Clear(bool bRemoveSlack = false)
    {
        if (ArraySize)
        {
            ::DestroyObjects<ElementType>(Allocator.GetAllocation(), ArraySize);
            ArraySize = 0;
        }

        if (bRemoveSlack)
        {
            Allocator.Free();
            ArrayMax = 0;
        }
    }

    /**
     * @brief         - Resets the container, optionally default constructing a new amount of elements.
     * @param NewSize - Number of elements to construct (default is 0)
     */
    void Reset(SIZETYPE NewSize = 0)
    {
        ::DestroyObjects<ElementType>(Allocator.GetAllocation(), ArraySize);
        if (NewSize)
        {
            InitializeEmpty(NewSize);
        }
        else
        {
            ArraySize = 0;
        }
    }

    /** 
     * @brief         - Resets the container, initializes with copies of a specific element.
     * @param NewSize - Number of elements to construct
     * @param Element - Element to copy-construct from
     */
    void Reset(SIZETYPE NewSize, const ElementType& Element)
    {
        ::DestroyObjects<ElementType>(Allocator.GetAllocation(), ArraySize);
        if (NewSize)
        {
            Initialize(NewSize, Element);
        }
        else
        {
            ArraySize = 0;
        }
    }

    /** 
     * @brief               - Resets the container, initializes with copies from a raw pointer array.
     * @param Elements      - Array to copy-construct from
     * @param NumElements   - Number of elements to copy
     */
    void Reset(const ElementType* Elements, SIZETYPE NumElements)
    {
        if (Elements != Allocator.GetAllocation())
        {
            ::DestroyObjects<ElementType>(Allocator.GetAllocation(), ArraySize);
            if (NumElements)
            {
                InitializeByCopy(Elements, NumElements, 0);
            }
            else
            {
                ArraySize = 0;
            }
        }
    }

    /** 
     * @brief            - Resets the container, copies elements from another array.
     * @param InputArray - Array to copy elements from
     */
    template<typename ArrayType>
    FORCEINLINE void Reset(const ArrayType& InputArray) requires(TIsTArrayType<ArrayType>::Value)
    {
        Reset(FArrayContainerHelper::Data(InputArray), FArrayContainerHelper::Size(InputArray));
    }

    /**
     * @brief            - Resets the container by moving elements from another array.
     * @param InputArray - Array to move elements from
     */
    FORCEINLINE void Reset(TArray&& InputArray)
    {
        InitializeByMove(Forward<TArray>(InputArray));
    }

    /** 
     * @brief          - Resets the container and copy-constructs from an initializer-list
     * @param InitList - Initializer-list to copy-construct elements from
     */
    FORCEINLINE void Reset(std::initializer_list<ElementType> InitList)
    {
        Reset(FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
    }

    /** 
     * @brief              - Fill the container with the specified value 
     * @param InputElement - Element to copy into all elements in the array
     */
    void Fill(const ElementType& InputElement)
    {
        ::AssignObjects(Allocator.GetAllocation(), InputElement, ArraySize);
    }

    /**
     * @brief         - Resizes the container with a new size, default constructing new elements or destructing excess elements
     * @param NewSize - The new size of the array
     */
    void Resize(SIZETYPE NewSize)
    {
        if (NewSize > ArraySize)
        {
            if (NewSize > ArrayMax)
            {
                ReserveUnchecked(NewSize);
            }

            // NewSize is always larger than array-size...
            const SIZETYPE NumElementsToConstruct = NewSize - ArraySize;
            // ...However, assert just in case
            CHECK(NumElementsToConstruct > 0);

            ElementType* LastElementPtr = Allocator.GetAllocation() + ArraySize;
            ::DefaultConstructObjects<ElementType>(LastElementPtr, NumElementsToConstruct);
            ArraySize = NewSize;
        }
        else if (NewSize < ArraySize)
        {
            const SIZETYPE NumElementsToDestruct = ArraySize - NewSize;
            CHECK(NumElementsToDestruct > 0);
            Pop(NumElementsToDestruct);
        }
    }

    /**
     * @brief         - Resizes the container with a new size without calling any constructors
     * @param NewSize - The new size of the array
     */
    void ResizeUninitialized(SIZETYPE NewSize)
    {
        if (NewSize > ArraySize)
        {
            if (NewSize > ArrayMax)
            {
                ReserveUnchecked(NewSize);
            }

            ArraySize = NewSize;
        }
        else if (NewSize < ArraySize)
        {
            const SIZETYPE NumElementsToDestruct = ArraySize - NewSize;
            CHECK(NumElementsToDestruct > 0);
            Pop(NumElementsToDestruct);
        }
    }

    /**
     * @brief             - Reallocate the array to a new capacity
     * @param NewCapacity - The new capacity of the allocated array
     */
    void Reserve(SIZETYPE NewCapacity)
    {
        if (NewCapacity != ArrayMax)
        {
            if (NewCapacity < ArraySize)
            {
                ::DestroyObjects<ElementType>(Allocator.GetAllocation() + NewCapacity, ArraySize - NewCapacity);
                ArraySize = NewCapacity;
            }

            ReserveUnchecked(NewCapacity);
        }
    }

    /** 
     * @brief      - Constructs a new element at the end of the array 
     * @param Args - Arguments for the constructor of the element
     * @return     - Returns a reference to the newly created element
     */
    template<typename... ArgTypes>
    FORCEINLINE ElementType& Emplace(ArgTypes&&... Args)
    {
        EnsureCapacity(ArraySize + 1);
        new(reinterpret_cast<void*>(Allocator.GetAllocation() + (ArraySize++))) ElementType(Forward<ArgTypes>(Args)...);
        return LastElement();
    }

    /**
     * @brief         - Appends a new element at the end of the array by copy
     * @param Element - Element to insert into the array by copy
     * @return        - Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Add(const ElementType& Element)
    {
        return Emplace(Element);
    }

    /**
     * @brief         - Appends a new element at the end of the array by move
     * @param Element - Element to insert into the array by move
     * @return        - Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Add(ElementType&& Element)
    {
        return Emplace(Forward<ElementType>(Element));
    }

    /**
     * @brief         - Appends a new element at the end of the array if it does not already exist (copy)
     * @param Element - Element to insert into the array by copy
     * @return        - Returns the index of the newly created element or the existing element
     */
    FORCEINLINE SIZETYPE AddUnique(const ElementType& Element)
    {
        const SIZETYPE Index = Find(Element);
        if (Index >= 0)
        {
            return Index;
        }

        Emplace(Element);
        return ArraySize - 1;
    }

    /**
     * @brief         - Appends a new element at the end of the array if it does not already exist (move)
     * @param Element - Element to insert into the array by move
     * @return        - Returns the index of the newly created element or the existing element
     */
    FORCEINLINE SIZETYPE AddUnique(ElementType&& Element)
    {
        const SIZETYPE Index = Find(Element);
        if (Index >= 0)
        {
            return Index;
        }

        Emplace(Forward<ElementType>(Element));
        return ArraySize - 1;
    }

    /**
     * @brief - Appends a new element at the end of the array without calling any constructor
     */
    FORCEINLINE void AddUninitialized()
    {
        AppendUninitialized(1);
    }

    /**
     * @brief - Appends a default-constructed element
     */
    FORCEINLINE ElementType& AddDefault()
    {
        return Emplace();
    }

    /**
     * @brief          - Constructs a new element at a specific position in the array
     * @param Position - Position of the new element
     * @param Args     - Arguments for the constructor of the element
     */
    template<typename... ArgTypes>
    FORCEINLINE void EmplaceAt(SIZETYPE Position, ArgTypes&&... Args)
    {
        CHECK(Position <= ArraySize);
        InsertUninitializedUnchecked(Position, 1);
        new(reinterpret_cast<void*>(Allocator.GetAllocation() + Position)) ElementType(Forward<ArgTypes>(Args)...);
        ArraySize++;
    }

    /**
     * @brief          - Insert a new element at a specific position in the array by copy
     * @param Position - Position of the new element
     * @param Element  - Element to copy into the position
     */
    FORCEINLINE void Insert(SIZETYPE Position, const ElementType& Element)
    {
        EmplaceAt(Position, Element);
    }

    /**
     * @brief          - Insert a new element at a specific position in the array by move
     * @param Position - Position of the new element
     * @param Element  - Element to move into the position
     */
    FORCEINLINE void Insert(SIZETYPE Position, ElementType&& Element)
    {
        EmplaceAt(Position, Forward<ElementType>(Element));
    }

    /**
     * @brief             - Insert an array at a specific position in the array
     * @param Position    - Position to insert elements
     * @param Elements    - Array to copy elements from
     * @param InNumElements - Number of elements to copy
     */
    void Insert(SIZETYPE Position, const ElementType* Elements, SIZETYPE InNumElements)
    {
        CHECK(Position <= ArraySize);
        CHECK(Elements != nullptr);

        InsertUninitializedUnchecked(Position, InNumElements);
        ::CopyConstructObjects<ElementType>(Allocator.GetAllocation() + Position, Elements, InNumElements);
        ArraySize += InNumElements;
    }

    /**
     * @brief          - Insert elements from an initializer list at a specific position in the array
     * @param Position - Position to insert elements
     * @param InitList - Initializer list to copy elements from
     */
    FORCEINLINE void Insert(SIZETYPE Position, std::initializer_list<ElementType> InitList)
    {
        Insert(Position, FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
    }

    /**
      * @brief          - Insert elements from another array at a specific position in the array
      * @param Position - Position to insert elements
      * @param InArray  - Array to copy elements from
      */
    template<typename ArrayType>
    FORCEINLINE void Insert(SIZETYPE Position, const ArrayType& InArray) requires(TIsTArrayType<ArrayType>::Value)
    {
        Insert(Position, FArrayContainerHelper::Data(InArray), FArrayContainerHelper::Size(InArray));
    }

    /**
     * @brief             - Insert a number of uninitialized elements in the array at a specific position
     * @param Position    - Start-position of the new elements
     * @param NumElements - Number of elements to insert
     */
    FORCEINLINE void InsertUninitialized(SIZETYPE Position, SIZETYPE NumElements)
    {
        CHECK(Position <= ArraySize);
        InsertUninitializedUnchecked(Position, NumElements);
        ArraySize += NumElements;
    }

    /**
     * @brief             - Append elements from a raw-pointer array at the end of the array
     * @param InElements  - Array to copy elements from
     * @param NumElements - Number of elements in the input array
     */
    void Append(const ElementType* InElements, SIZETYPE NumElements)
    {
        if (NumElements > 0)
        {
            CHECK(InElements != nullptr);
            EnsureCapacity(ArraySize + NumElements);
            ::CopyConstructObjects<ElementType>(Allocator.GetAllocation() + ArraySize, InElements, NumElements);
            ArraySize += NumElements;
        }
    }

    /**
     * @brief       - Append another array at the end of the array
     * @param Other - Array to copy elements from
     */
    template<typename ArrayType>
    FORCEINLINE void Append(const ArrayType& Other) requires(TIsTArrayType<ArrayType>::Value)
    {
        Append(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other));
    }

    /**
     * @brief          - Append an initializer list at the end of the array
     * @param InitList - Initializer list to copy elements from
     */
    FORCEINLINE void Append(std::initializer_list<ElementType> InitList)
    {
        Append(FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
    }

    /**
     * @brief         - Create a number of uninitialized elements at the end of the array
     * @param NewSize - Number of elements to append
     */
    FORCEINLINE void AppendUninitialized(SIZETYPE NumElements)
    {
        const SIZETYPE NewSize = ArraySize + NumElements;
        EnsureCapacity(NewSize);
        ArraySize = NewSize;
    }

    /**
     * @brief             - Remove and destroy a number of elements from the back
     * @param NumElements - Number of elements to destroy from the end
     */
    void Pop(SIZETYPE NumElements = 1)
    {
        CHECK(!IsEmpty());
        SIZETYPE NewArraySize = ArraySize - NumElements;
        ::DestroyObjects<ElementType>(Allocator.GetAllocation() + NewArraySize, NumElements);
        ArraySize = NewArraySize;
    }

    /**
     * @brief             - Remove a range of elements starting at position
     * @param Position    - Position to start removing elements
     * @param NumElements - Number of elements to remove
     */
    void RemoveAt(SIZETYPE Position, SIZETYPE NumElements)
    {
        CHECK(Position + NumElements <= ArraySize);

        if (NumElements)
        {
            ElementType* TempPositionData = Allocator.GetAllocation() + Position;
            ::DestroyObjects<ElementType>(TempPositionData, NumElements);
            ::RelocateObjects<ElementType>(TempPositionData, TempPositionData + NumElements, ArraySize - (Position + NumElements));
            ArraySize -= NumElements;
        }
    }

    /**
     * @brief          - Removes the element at the specified position
     * @param Position - Position of element to remove
     */
    FORCEINLINE void RemoveAt(SIZETYPE Position)
    {
        RemoveAt(Position, 1);
    }

    /**
     * @brief         - Search the array and remove all instances of the element
     * @param Element - Element to remove
     * @return        - Returns true if the element was found and removed, false otherwise
     */
    bool Remove(const ElementType& Element)
    {
        bool bRemoved = false;
        ElementType* Array = Allocator.GetAllocation();
        for (SIZETYPE Index = 0; Index < ArraySize;)
        {
            if (Element == Array[Index])
            {
                RemoveAt(Index);
                bRemoved = true;
            }
            else
            {
                ++Index;
            }
        }

        return bRemoved;
    }

    /**
     * @brief         - Search the array and remove all elements satisfying the predicate
     * @param Predicate - Callable that determines if an element should be removed
     */
    template<typename PredicateType>
    FORCEINLINE void RemoveAll(PredicateType&& Predicate)
    {
        ElementType* Array = Allocator.GetAllocation();
        for (SIZETYPE Index = 0; Index < ArraySize;)
        {
            if (Predicate(Array[Index]))
            {
                RemoveAt(Index);
            }
            else
            {
                ++Index;
            }
        }
    }

    /**
     * @brief           - Returns the index of an element if it is present in the array, or INVALID_INDEX if not found
     * @param InElement - Element to search for
     * @return          - The index of the element if found or INVALID_INDEX if not
     */
    NODISCARD FORCEINLINE SIZETYPE Find(const ElementType& InElement) const
    {
        return FindForward([&](const ElementType& Element)
        {
            return Element == InElement;
        });
    }

    /**
     * @brief           - Returns the index of the element that satisfies the conditions of a comparator
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindWithPredicate(PredicateType&& Predicate) const
    {
        return FindForward(Forward<PredicateType>(Predicate));
    }

    /**
     * @brief         - Returns the index of the last occurrence of an element, or INVALID_INDEX if not found
     * @param Element - Element to search for
     * @return        - The index of the element if found or INVALID_INDEX if not
     */
    NODISCARD FORCEINLINE SIZETYPE FindLast(const ElementType& InElement) const
    {
        return FindReverse([&](const ElementType& Element)
        {
            return Element == InElement;
        });
    }

    /**
     * @brief           - Returns the index of the last element that satisfies the conditions of a comparator
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindLastWithPredicate(PredicateType&& Predicate) const
    {
        return FindReverse(Forward<PredicateType>(Predicate));
    }

    /**
     * @brief         - Check if an element exists in the array
     * @param Element - Element to check for
     * @return        - Returns true if the element is found in the array, false otherwise
     */
    NODISCARD FORCEINLINE bool Contains(const ElementType& Element) const
    {
        return Find(Element) != INVALID_INDEX;
    }

    /**
     * @brief           - Check if an element satisfying the predicate exists in the array
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - Returns true if the predicate returned true for any element
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE bool ContainsWithPredicate(PredicateType&& Predicate) const
    {
        return FindWithPredicate(Forward<PredicateType>(Predicate)) != INVALID_INDEX;
    }

    /**
     * @brief        - Perform a function on each element in the array (non-const)
     * @param Lambda - Callable that takes one element and performs some operation on it
     */
    template<class LambdaType>
    void Foreach(LambdaType&& Lambda)
    {
        for (ElementType* RESTRICT Current = Allocator.GetAllocation(), *RESTRICT End = Current + ArraySize; Current != End; ++Current)
        {
            Lambda(*Current);
        }
    }

    /**
     * @brief        - Perform a function on each element in the array (const version)
     * @param Lambda - Callable that takes one element and performs some operation on it
     */
    template<class LambdaType>
    void Foreach(LambdaType&& Lambda) const
    {
        for (const ElementType* RESTRICT Current = Allocator.GetAllocation(), *RESTRICT End = Current + ArraySize; Current != End; ++Current)
        {
            Lambda(*Current);
        }
    }

    /**
     * @brief             - Swap two elements within the array
     * @param FirstIndex  - Index of the first element to swap
     * @param SecondIndex - Index of the second element to swap
     */
    FORCEINLINE void Swap(SIZETYPE FirstIndex, SIZETYPE SecondIndex)
    {
        CHECK(IsValidIndex(FirstIndex));
        CHECK(IsValidIndex(SecondIndex));
        ElementType* Array = Allocator.GetAllocation();
        ::Swap(Array[FirstIndex], Array[SecondIndex]);
    }

    /**
     * @brief - Shrink the allocation to perfectly fit the size of the array
     */
    FORCEINLINE void Shrink()
    {
        Reserve(ArraySize);
    }

    /**
     * @brief - Reverses the order of the array
     */
    void Reverse()
    {
        const SIZETYPE HalfSize = ArraySize / 2;
        ElementType* Array = Allocator.GetAllocation();
        for (SIZETYPE Index = 0; Index < HalfSize; ++Index)
        {
            const SIZETYPE ReverseIndex = ArraySize - Index - 1;
            ::Swap(Array[Index], Array[ReverseIndex]);
        }
    }

    /**
     * @brief - Sort the array using the quick-sort algorithm, assumes that the ElementType has the '<' operator
     */
    FORCEINLINE void Sort()
    {
        SortInternal(0, LastElementIndex(), [](const ElementType& First, const ElementType& Second)
        {
            return (First < Second);
        });
    }

    /**
     * @brief - Sort the array using the quick-sort algorithm with a custom comparator
     */
    template<typename PredicateType>
    FORCEINLINE void SortWithPredicate(PredicateType&& Predicate)
    {
        SortInternal(0, LastElementIndex(), Forward<PredicateType>(Predicate));
    }

    /**
     * @brief         - Checks that the pointer is a part of the array
     * @param Address - Address to check.
     * @return        - Returns true if the address belongs to the array
     */
    NODISCARD FORCEINLINE bool CheckAddress(const ElementType* Address) const
    {
        const ElementType* Array = Allocator.GetAllocation();
        return Address >= Array && Address < (Array + ArrayMax);
    }

    /**
     * @brief  - Checks if an index is a valid index
     * @return - Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SIZETYPE Index) const
    {
        return Index >= 0 && Index < ArraySize;
    }

    /**
     * @brief  - Check if the container contains any elements
     * @return - Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return ArraySize == 0;
    }

    /**
     * @brief  - Retrieve the first element of the array (non-const)
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE ElementType& FirstElement()
    {
        CHECK(!IsEmpty());
        ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief  - Retrieve the first element of the array (const)
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const ElementType& FirstElement() const
    {
        CHECK(!IsEmpty());
        const ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief  - Retrieve the last element of the array (non-const)
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE ElementType& LastElement()
    {
        CHECK(!IsEmpty());
        ElementType* Array = Allocator.GetAllocation();
        return Array[LastElementIndex()];
    }

    /**
     * @brief  - Retrieve the last element of the array (const)
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const ElementType& LastElement() const
    {
        CHECK(!IsEmpty());
        const ElementType* Array = Allocator.GetAllocation();
        return Array[LastElementIndex()];
    }

    /**
     * @brief  - Retrieve the data of the array (non-const)
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE ElementType* Data()
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief  - Retrieve the data of the array (const)
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const ElementType* Data() const
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns the index to the last element of the array
     */
    NODISCARD FORCEINLINE SIZETYPE LastElementIndex() const
    {
        return ArraySize ? ArraySize - 1 : 0;
    }

    /**
     * @brief  - Retrieve the size of the container
     * @return - Returns the size of the container
     */
    NODISCARD FORCEINLINE SIZETYPE Size() const
    {
        return ArraySize;
    }

    /**
     * @brief  - Retrieve the stride of each element of the container
     * @return - Returns the stride of each element of the container
     */
    NODISCARD FORCEINLINE constexpr SIZETYPE Stride() const
    {
        return sizeof(ElementType);
    }

    /**
     * @brief  - Retrieve the size of the container in bytes
     * @return - Returns the size of the container in bytes
     */
    NODISCARD FORCEINLINE SIZETYPE SizeInBytes() const
    {
        return ArraySize * sizeof(ElementType);
    }

    /**
     * @brief  - Retrieve the capacity of the container
     * @return - Returns the capacity of the container
     */
    NODISCARD FORCEINLINE SIZETYPE Capacity() const
    {
        return ArrayMax;
    }

    /**
     * @brief  - Retrieve the capacity of the container in bytes
     * @return - Returns the capacity of the container in bytes
     */
    NODISCARD FORCEINLINE SIZETYPE CapacityInBytes() const
    {
        return ArrayMax * sizeof(ElementType);
    }

    /**
     * @brief  - Retrieve the allocator (non-const)
     * @return - Returns a reference to the allocator
     */
    NODISCARD FORCEINLINE AllocatorType& GetAllocator()
    {
        return Allocator;
    }

    /**
     * @brief  - Retrieve the allocator (const)
     * @return - Returns a const reference to the allocator
     */
    NODISCARD FORCEINLINE const AllocatorType& GetAllocator() const
    {
        return Allocator;
    }

public:

    // Heap Functions

    /**
     * @brief - Create a heap from the array 
     */
    void Heapify()
    {
        if (ArraySize == 0)
        {
            return;
        }

        const SIZETYPE StartIndex = (ArraySize / 2) - 1;
        for (SIZETYPE Index = StartIndex; Index >= 0; --Index)
        {
            Heapify(ArraySize, Index);
            if (Index == 0)
            {
                break;
            }
        }
    }

    /**
     * @brief  - Retrieve the top of the heap. The same as the first element.
     * @return - A reference to the element at the top of the heap
     */
    NODISCARD FORCEINLINE ElementType& HeapTop()
    {
        CHECK(!IsEmpty());
        ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief  - Retrieve the top of the heap. The same as the first element.
     * @return - A reference to the element at the top of the heap
     */
    NODISCARD FORCEINLINE const ElementType& HeapTop() const
    {
        CHECK(!IsEmpty());
        const ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief         - Inserts a new element into the heap by copy
     * @param Element - Element to insert into the heap by copy
     */
    FORCEINLINE void HeapPush(const ElementType& Element)
    {
        Insert(0, Element);
        Heapify(ArraySize, 0);
    }

    /**
     * @brief         - Inserts a new element into the heap by move
     * @param Element - Element to insert into the heap by move
     */
    FORCEINLINE void HeapPush(ElementType&& Element)
    {
        Insert(0, Forward<ElementType>(Element));
        Heapify(ArraySize, 0);
    }

    /**
     * @brief            - Remove the top of the heap and retrieve the element on top
     * @param OutElement - Reference that the top element will be copied to
     */
    FORCEINLINE void HeapPop(ElementType& OutElement)
    {
        OutElement = HeapTop();
        HeapPop();
    }

    /**
     * @brief - Remove the top of the heap
     */
    FORCEINLINE void HeapPop()
    {
        CHECK(!IsEmpty());
        RemoveAt(0);
        Heapify();
    }

    /**
     * @brief - Performs heap sort on the array (assuming the operator> exists for the elements)
     */
    void HeapSort()
    {
        Heapify();

        ElementType* Array = Allocator.GetAllocation();
        for (SIZETYPE Index = ArraySize - 1; Index > 0; --Index)
        {
            ::Swap<ElementType>(Array[0], Array[Index]);
            Heapify(Index, 0);
        }
    }

public:

    /**
     * @brief       - Copy-assignment operator
     * @param Other - Array to copy
     * @return      - A reference to this container
     */
    FORCEINLINE TArray& operator=(const TArray& Other)
    {
        if (this != &Other)
        {
            Reset(Other);
        }

        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - Array to move
     * @return      - A reference to this container
     */
    FORCEINLINE TArray& operator=(TArray&& Other)
    {
        if (this != &Other)
        {
            InitializeByMove(Forward<TArray>(Other));
        }

        return *this;
    }

    /**
     * @brief       - Assignment operator that takes an initializer-list
     * @param Other - An initializer list to replace the current contents with
     * @return      - A reference to this container
     */
    FORCEINLINE TArray& operator=(std::initializer_list<ElementType> Other)
    {
        Reset(Other);
        return *this;
    }

    /**
     * @brief       - Comparison operator that compares all elements in the array
     * @param Other - Array to compare with
     * @return        - Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    NODISCARD bool operator==(const ArrayType& Other) const requires(TIsTArrayType<ArrayType>::Value)
    {
        return ArraySize == FArrayContainerHelper::Size(Other) ?
            ::CompareObjects<ElementType>(Allocator.GetAllocation(), FArrayContainerHelper::Data(Other), ArraySize) :
            false;
    }

    /**
     * @brief       - Comparison operator that compares all elements in the array
     * @param Other - Array to compare with
     * @return        - Returns true if any element is NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator!=(const ArrayType& Other) const requires(TIsTArrayType<ArrayType>::Value)
    {
        return !(*this == Other);
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index (non-const)
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SIZETYPE Index)
    {
        return Allocator.GetAllocation()[Index];
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index (const)
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SIZETYPE Index) const
    {
        return Allocator.GetAllocation()[Index];
    }

public:

    /**
     * @brief       - Append another array at the end of the array
     * @param Other - Array to copy elements from
     * @return      - Reference to this array
     */
    template<typename ArrayType>
    FORCEINLINE TArray& operator+=(const ArrayType& Other) requires(TIsTArrayType<ArrayType>::Value)
    {
        Append(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other));
        return *this;
    }

    /**
     * @brief          - Append an initializer-list at the end of the array
     * @param InitList - Initializer-list to copy elements from
     * @return         - Reference to this array
     */
    FORCEINLINE TArray& operator+=(std::initializer_list<ElementType> InitList)
    {
        Append(FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
        return *this;
    }

public:
    /**
     * @brief       - Friend operator to concatenate two arrays
     * @param LHS - Left-hand side array
     * @param RHS - Right-hand side array
     * @return    - A new array containing elements from both arrays
     */
    NODISCARD FORCEINLINE friend TArray operator+(const TArray& LHS, const TArray& RHS)
    {
        TArray NewArray(LHS, RHS.Size());
        NewArray.Append(RHS);
        return NewArray;
    }

public:

    // Iterators

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - An iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType Iterator()
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve a const iterator to the beginning of the array
     * @return - A const iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve a reverse iterator to the end of the array
     * @return - A reverse iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator()
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * @brief  - Retrieve a const reverse iterator to the end of the array
     * @return - A const reverse iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseIterator() const
    {
        return ReverseConstIteratorType(*this, Size());
    }

public:

    // STL Iterators

    NODISCARD FORCEINLINE IteratorType      begin()       { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return ConstIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       { return IteratorType(*this, ArraySize); }
    NODISCARD FORCEINLINE ConstIteratorType end() const { return ConstIteratorType(*this, ArraySize); }

private:

    // Initialization Helpers

    FORCEINLINE void CreateUninitialized(SIZETYPE NumElements)
    {
        if (ArrayMax < NumElements)
        {
            Allocator.Realloc(ArrayMax, NumElements);
            ArrayMax = NumElements;
        }

        ArraySize = NumElements;
    }

    void InitializeEmpty(SIZETYPE NumElements)
    {
        CreateUninitialized(NumElements);
        ::DefaultConstructObjects<ElementType>(Allocator.GetAllocation(), NumElements);
    }

    void Initialize(SIZETYPE NumElements, const ElementType& Element)
    {
        CreateUninitialized(NumElements);
        ::ConstructObjectsFrom<ElementType>(Allocator.GetAllocation(), NumElements, Element);
    }

    void InitializeByCopy(const ElementType* Elements, SIZETYPE NumElements, SIZETYPE ExtraCapacity)
    {
        const SIZETYPE NewSize = NumElements + ExtraCapacity;
        CreateUninitialized(NewSize);
        ::CopyConstructObjects<ElementType>(Allocator.GetAllocation(), Elements, NumElements);
    }

    void InitializeByMove(TArray&& FromArray)
    {
        if (this != &FromArray)
        {
            // Destroy current elements
            ::DestroyObjects<ElementType>(Allocator.GetAllocation(), ArraySize);
            // Move allocator resources
            Allocator.MoveFrom(Move(FromArray.Allocator));

            // Transfer size and capacity
            ArraySize = FromArray.ArraySize;
            ArrayMax = FromArray.ArrayMax;
            FromArray.ArraySize = 0;
            FromArray.ArrayMax  = 0;
        }
    }

    void ReserveUnchecked(const SIZETYPE NewCapacity)
    {
        if constexpr (TNot<TIsReallocatable<ElementType>>::Value)
        {
            if (ArrayMax)
            {
                // For non-trivial objects, reallocate with proper handling
                AllocatorType NewAllocator;
                NewAllocator.Realloc(ArrayMax, NewCapacity);
                if (ArraySize)
                {
                    ::RelocateObjects<ElementType>(NewAllocator.GetAllocation(), Allocator.GetAllocation(), ArraySize);
                }

                Allocator.MoveFrom(Move(NewAllocator));
            }
            else
            {
                Allocator.Realloc(ArrayMax, NewCapacity);
            }
        }
        else
        {
            Allocator.Realloc(ArrayMax, NewCapacity);
        }

        ArrayMax = NewCapacity;
    }

    void InsertUninitializedUnchecked(const SIZETYPE Position, const SIZETYPE InNumElements)
    {
        EnsureCapacity(ArraySize + InNumElements);
        ElementType* const CurrentAddress = Allocator.GetAllocation() + Position;
        ::RelocateObjects<ElementType>(CurrentAddress + InNumElements, CurrentAddress, ArraySize - Position);
    }

    void FORCEINLINE EnsureCapacity(SIZETYPE RequiredCapacity)
    {
        if (RequiredCapacity > ArrayMax)
        {
            const SIZETYPE NewCapacity = CalculateGrowth(RequiredCapacity, ArrayMax);
            ReserveUnchecked(NewCapacity);
        }
    }

    void Heapify(SIZETYPE InSize, SIZETYPE Index)
    {
        SIZETYPE StartIndex = Index;
        SIZETYPE Largest    = Index;

        ElementType* Array = Allocator.GetAllocation();
        while (true)
        {
            const SIZETYPE Left  = LeftIndex(StartIndex);
            const SIZETYPE Right = RightIndex(StartIndex);

            if (Left < InSize && Array[Left] > Array[Largest])
            {
                Largest = Left;
            }

            if (Right < InSize && Array[Right] > Array[Largest])
            {
                Largest = Right;
            }

            if (Largest != StartIndex)
            {
                ::Swap<ElementType>(Array[StartIndex], Array[Largest]);
                StartIndex = Largest;
            }
            else
            {
                break;
            }
        }
    }

    NODISCARD FORCEINLINE static SIZETYPE LeftIndex(SIZETYPE Index)
    {
        return 2 * Index + 1;
    }

    NODISCARD FORCEINLINE static SIZETYPE RightIndex(SIZETYPE Index)
    {
        return 2 * Index + 2;
    }

    // Calculate how much the array should grow, will always be at least one
    NODISCARD static SIZETYPE CalculateGrowth(SIZETYPE NumElements, SIZETYPE CurrentCapacity)
    {
        constexpr SIZETYPE FirstAlloc = 4;

        SIZETYPE NewSize;
        if (CurrentCapacity)
        {
            NewSize = CurrentCapacity + NumElements + (CurrentCapacity >> 1);
        }
        else if (NumElements > FirstAlloc)
        {
            NewSize = NumElements;
        }
        else
        {
            NewSize = FirstAlloc;
        }

        return NewSize;
    }

    NODISCARD SIZETYPE FindForward(auto&& Predicate) const
    {
        const ElementType* Start = Allocator.GetAllocation();
        const ElementType* End  = Start + ArraySize;
        for (const ElementType* Current = Start; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    NODISCARD SIZETYPE FindReverse(auto&& Predicate) const
    {
        const ElementType* Start   = Allocator.GetAllocation();
        const ElementType* End     = Start;
        const ElementType* Current = Start + ArraySize;
        while (Current != End)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(Current - Start);
            }
        }

        return INVALID_INDEX;
    }

    template<typename PredicateType>
    SIZETYPE SortPartition(SIZETYPE First, SIZETYPE Last, PredicateType&& Predicate)
    {
        ElementType* Array = Allocator.GetAllocation();

        // Select the last element as pivot
        const SIZETYPE Pivot = Last;
        SIZETYPE Index = First;
        for (SIZETYPE Current = First; Current < Last; ++Current)
        {
            if (Predicate(Array[Current], Array[Pivot]))
            {
                ::Swap(Array[Index], Array[Current]);
                Index++;
            }
        }

        ::Swap(Array[Index], Array[Pivot]);
        return Index;
    }

    template<typename PredicateType>
    void SortInternal(SIZETYPE First, SIZETYPE Last, PredicateType&& Predicate)
    {
        if (First >= Last)
        {
            return;
        }

        // Threshold for switching to insertion sort
        constexpr SIZETYPE Threshold = 24;
        if ((Last - First + 1) >= Threshold)
        {
            const SIZETYPE Pivot = SortPartition(First, Last, Predicate);
            SortInternal(First, Pivot - 1, Predicate);
            SortInternal(Pivot + 1, Last, Predicate);
        }
        else
        {
            ElementType* Array = Allocator.GetAllocation();
            for (SIZETYPE Current = First + 1; Current <= Last; ++Current) 
            {
                SIZETYPE Index = Current;
                while (Index > First && Predicate(Array[Index], Array[Index - 1]))
                {
                    ::Swap(Array[Index], Array[Index - 1]);
                    Index--;
                }
            }
        }
    }

private:
    AllocatorType Allocator;
    SIZETYPE      ArraySize = 0;
    SIZETYPE      ArrayMax  = 0;
};

template<typename T, typename AllocatorType>
struct TIsTArrayType<TArray<T, AllocatorType>>
{
    inline static constexpr bool Value = true;
};

template<typename T, typename AllocatorType>
struct TIsContiguousContainer<TArray<T, AllocatorType>>
{
    inline static constexpr bool Value = true;
};
