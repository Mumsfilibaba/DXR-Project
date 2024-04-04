#pragma once
#include "Allocators.h"
#include "UniquePtr.h"
#include "ArrayView.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/Functional.h"
#include "Core/Math/Random.h"

template<typename ElementType, typename AllocatorType = TDefaultArrayAllocator<ElementType>>
class TArray
{
public:
    typedef int32 SizeType;
    static_assert(TIsSigned<SizeType>::Value, "TArray only supports a SizeType that's signed");

    typedef TArrayIterator<TArray, ElementType>                    IteratorType;
    typedef TArrayIterator<const TArray, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TArray, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TArray, const ElementType> ReverseConstIteratorType;

    enum : SizeType
    { 
        INVALID_INDEX = -1
    };

public:

    /** @brief - Default constructor */
    TArray() = default;

    /** 
     * @brief        - Constructor that default creates a certain number of elements 
     * @param InSize - Number of elements to construct
     */
    FORCEINLINE explicit TArray(SizeType InSize) noexcept
    {
        InitializeEmpty(InSize);
    }

    /**
     * @brief         - Constructor that Allocates the specified amount of elements, and initializes them to the same value 
     * @param InSize  - Number of elements to construct
     * @param Element - Element to copy into all positions of the array
     */
    FORCEINLINE TArray(SizeType InSize, const ElementType& Element) noexcept
    {
        Initialize(InSize, Element);
    }

    /**
     * @brief               - Constructor that creates an array from a raw pointer array 
     * @param InElements    - Pointer to the start of the array to copy from
     * @param InNumElements - Number of elements in 'InputArray', which also is the resulting size of the constructed array
     * @param InSlack       - Extra number of elements to allocate space for
     */
    FORCEINLINE TArray(const ElementType* InElements, SizeType InNumElements, SizeType InSlack = 0) noexcept
    {
        InitializeByCopy(InElements, InNumElements, InSlack);
    }

    /** 
     * @brief       - Copy-constructor
     * @param Other - Array to copy from
     */
    FORCEINLINE TArray(const TArray& Other) noexcept
    {
        InitializeByCopy(Other.Data(), Other.Size(), 0);
    }

    /** 
     * @brief         - Copy-constructor
     * @param Other   - Array to copy from
     * @param InSlack - Extra number of elements to allocate space for
     */
    FORCEINLINE TArray(const TArray& Other, SizeType InSlack) noexcept
    {
        InitializeByCopy(Other.Data(), Other.Size(), InSlack);
    }

    /**
     * @brief       - Copy-constructor from another type of array
     * @param Other - Array to copy from
     */
    template<typename ArrayType>
    FORCEINLINE explicit TArray(const ArrayType& Other) noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        InitializeByCopy(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other), 0);
    }

    /** 
     * @brief       - Move-constructor 
     * @param Other - Array to move elements from
     */
    FORCEINLINE TArray(TArray&& Other) noexcept
    {
        InitializeByMove(::Forward<TArray>(Other));
    }

    /** 
     * @brief          - Constructor that creates an array from an std::initializer_list
     * @param InitList - Initializer list containing all elements to construct the array from
     */
    FORCEINLINE TArray(std::initializer_list<ElementType> InitList) noexcept
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
    void Clear(bool bRemoveSlack = false) noexcept
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
     * @brief         - Resets the container, but does not deallocate the memory. Takes an optional parameter to  default construct a new amount of elements.
     * @param NewSize - Number of elements to construct
     */
    void Reset(SizeType InSize = 0) noexcept
    {
        ::DestroyObjects<ElementType>(Allocator.GetAllocation(), ArraySize);
        if (InSize)
        {
            InitializeEmpty(InSize);
        }
        else
        {
            ArraySize = 0;
        }
    }

    /** 
     * @brief         - Resets the container, but does not deallocate the memory. Takes an optional parameter to default construct a new amount of elements from a single element.
     * @param NewSize - Number of elements to construct
     * @param Element - Element to copy-construct from
     */
    void Reset(SizeType NewSize, const ElementType& Element) noexcept
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
     * @brief               - Resets the container, but does not deallocate the memory. Takes in pointer to copy-construct elements from.
     * @param Elements      - Array to copy-construct from
     * @param InNumElements - Number of elements from
     */
    void Reset(const ElementType* Elements, SizeType NumElements) noexcept
    {
        if (NumElements <= 0)
        {
            return;
        }

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
     * @brief            - Resets the container, but does not deallocate the memory. Creates a new array from another array which can be of another type of array.
     * @param InputArray - Array to copy-construct from
     */
    template<typename ArrayType>
    FORCEINLINE void Reset(const ArrayType& InputArray) noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        Reset(FArrayContainerHelper::Data(InputArray), FArrayContainerHelper::Size(InputArray));
    }

    /**
     * @brief            - Resets the container by moving elements from another array to this one.
     * @param InputArray - Array to copy-construct elements from
     */
    FORCEINLINE void Reset(TArray&& InputArray) noexcept
    {
        InitializeByMove(::Forward<TArray>(InputArray));
    }

    /** 
     * @brief          - Resets the container and copy-construct a new array from an initializer-list
     * @param InitList - Initializer-list to copy-construct elements from
     */
    FORCEINLINE void Reset(std::initializer_list<ElementType> InitList) noexcept
    {
        Reset(FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
    }

    /** 
     * @brief              - Fill the container with the specified value 
     * @param InputElement - Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ::AssignObjects(Allocator.GetAllocation(), InputElement, ArraySize);
    }

    /**
     * @brief         - Resizes the container with a new size, and default constructs them
     * @param NewSize - The new size of the array
     */
    void Resize(SizeType NewSize) noexcept
    {
        if (NewSize > ArraySize)
        {
            if (NewSize > ArrayMax)
            {
                ReserveUnchecked(NewSize);
            }

            // NewSize is always larger than array-size...
            const SizeType NumElementsToConstruct = NewSize - ArraySize;
            // ...However, assert just in case
            CHECK(NumElementsToConstruct > 0);

            ElementType* LastElementPtr = Allocator.GetAllocation() + ArraySize;
            ::DefaultConstructObjects<ElementType>(LastElementPtr, NumElementsToConstruct);
            ArraySize = NewSize;
        }
        else if (NewSize < ArraySize)
        {
            const SizeType NumElementsToDestruct = ArraySize - NewSize;
            CHECK(NumElementsToDestruct > 0);
            Pop(NumElementsToDestruct);
        }
    }

    /**
     * @brief         - Resizes the container with a new size without calling any constructors
     * @param NewSize - The new size of the array
     */
    void ResizeUninitialized(SizeType NewSize) noexcept
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
            const SizeType NumElementsToDestruct = ArraySize - NewSize;
            CHECK(NumElementsToDestruct > 0);
            Pop(NumElementsToDestruct);
        }
    }

    /**
     * @brief             - Reallocate the array to a new capacity
     * @param NewCapacity - The new capacity of the allocated array
     */
    void Reserve(SizeType NewCapacity) noexcept
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
    ElementType& Emplace(ArgTypes&&... Args) noexcept
    {
        EnsureCapacity(ArraySize + 1);
        new(reinterpret_cast<void*>(Allocator.GetAllocation() + (ArraySize++))) ElementType(::Forward<ArgTypes>(Args)...);
        return LastElement();
    }

    /**
     * @brief         - Appends a new element at the end of the array
     * @param Element - Element to insert into the array by copy
     * @return        - Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Add(const ElementType& Element) noexcept
    {
        return Emplace(Element);
    }

    /**
     * @brief         - Appends a new element at the end of the array
     * @param Element - Element to insert into the array by move
     * @return        - Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Add(ElementType&& Element) noexcept
    {
        return Emplace(::Forward<ElementType>(Element));
    }

    /**
     * @brief         - Appends a new element at the end of the array if the element does not already exist
     * @param Element - Element to insert into the array by copy
     * @return        - Returns a reference to the newly created element or element equal to Element
     */
    SizeType AddUnique(const ElementType& Element) noexcept
    {
        const SizeType Index = Find(Element);
        if (Index >= 0)
        {
            return Index;
        }

        Emplace(Element);
        return ArraySize - 1;
    }

    /**
     * @brief         - Appends a new element at the end of the array if the element does not already exist
     * @param Element - Element to insert into the array by copy
     * @return        - Returns a reference to the newly created element or element equal to Element
     */
    SizeType AddUnique(ElementType&& Element) noexcept
    {
        const SizeType Index = Find(Element);
        if (Index >= 0)
        {
            return Index;
        }

        Emplace(::Forward<ElementType>(Element));
        return ArraySize - 1;
    }

    /**
     * @brief - Appends a new element at the end of the array, but does not call any constructor
     */
    FORCEINLINE void AddUninitialized() noexcept
    {
        AppendUninitialized(1);
    }

    /**
     * @brief          - Constructs a new element at a specific position in the array
     * @param Position - Position of the new element
     * @param Args     - Arguments for the constructor of the element
     */
    template<typename... ArgTypes>
    void EmplaceAt(SizeType Position, ArgTypes&&... Args) noexcept
    {
        CHECK(Position <= ArraySize);
        InsertUninitializedUnchecked(Position, 1);
        new(reinterpret_cast<void*>(Allocator.GetAllocation() + Position)) ElementType(::Forward<ArgTypes>(Args)...);
        ArraySize++;
    }

    /**
     * @brief          - Insert a new element at a specific position in the array
     * @param Position - Position of the new element
     * @param Element  - Element to copy into the position
     */
    FORCEINLINE void Insert(SizeType Position, const ElementType& Element) noexcept
    {
        EmplaceAt(Position, Element);
    }

    /**
     * @brief          - Insert a new element at the a specific position in the array by moving
     * @param Position - Position of the new element
     * @param Element  - Element to move into the position
     */
    FORCEINLINE void Insert(SizeType Position, ElementType&& Element) noexcept
    {
        EmplaceAt(Position, ::Forward<ElementType>(Element));
    }

    /**
     * @brief             - Insert an array at a specific position in the array
     * @param Position    - Position of the new element
     * @param InputArray  - Array to copy into the array
     * @param NumElements - Number of elements in the input-array
     */
    void Insert(SizeType Position, const ElementType* Elements, SizeType InNumElements) noexcept
    {
        CHECK(Position <= ArraySize);
        CHECK(Elements != nullptr);

        InsertUninitializedUnchecked(Position, InNumElements);
        ::CopyConstructObjects<ElementType>(Allocator.GetAllocation() + Position, Elements, InNumElements);
        ArraySize += InNumElements;
    }

    /**
     * @brief          - Insert elements from a initializer list at a specific position in the array
     * @param Position - Position of the new element
     * @param InitList - Initializer list to insert into the array
     */
    FORCEINLINE void Insert(SizeType Position, std::initializer_list<ElementType> InitList) noexcept
    {
        Insert(Position, FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
    }

    /**
      * @brief          - Insert elements from another array at a specific position in the array, which can be of another array-type.
      * @param Position - Position of the new element
      * @param InArray  - Array to copy elements from
      */
    template<typename ArrayType>
    FORCEINLINE void Insert(SizeType Position, const ArrayType& InArray) noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        Insert(Position, FArrayContainerHelper::Data(InArray), FArrayContainerHelper::Size(InArray));
    }

    /**
     * @brief             - Insert a number of uninitialized elements in the array at a specific position
     * @param Position    - Start-position of the new elements
     * @param NumElements - Number of elements to insert
     */
    void InsertUninitialized(SizeType Position, SizeType NumElements) noexcept
    {
        CHECK(Position <= ArraySize);
        InsertUninitializedUnchecked(Position, NumElements);
        ArraySize += NumElements;
    }

    /**
     * @brief             - Insert an array from a raw-pointer at the end of the array
     * @param InElements  - Array to copy elements from
     * @param NumElements - Number of elements in the input-array
     */
    void Append(const ElementType* InElements, SizeType NumElements) noexcept
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
     * @brief       - Insert another array at the end of the array, which can be of another array-type
     * @param Other - Array to copy elements from
     */
    template<typename ArrayType>
    FORCEINLINE void Append(const ArrayType& Other) noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        Append(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other));
    }

    /**
     * @brief          - Insert an initializer-list at the end of the array, which can be of another array-type
     * @param InitList - Initializer-list to copy elements from
     */
    FORCEINLINE void Append(std::initializer_list<ElementType> InitList) noexcept
    {
        Append(FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
    }

    /**
     * @brief         - Create a number of uninitialized elements at the end of the array
     * @param NewSize - Number of elements to append
     */
    FORCEINLINE void AppendUninitialized(SizeType NumElements) noexcept
    {
        const SizeType NewSize = ArraySize + NumElements;
        EnsureCapacity(NewSize);
        ArraySize = NewSize;
    }

    /**
     * @brief             - Remove and destroy a number of elements from the back
     * @param NumElements - Number of elements destroy from the end
     */
    FORCEINLINE void Pop(SizeType NumElements = 1) noexcept
    {
        CHECK(!IsEmpty());
        const SizeType NewArraySize = ArraySize - NumElements;
        ::DestroyObjects<ElementType>(Allocator.GetAllocation() + NewArraySize, NumElements);
        ArraySize = NewArraySize;
    }

    /**
     * @brief             - Remove a range of elements starting at position
     * @param Position    - Position of the array to start remove elements from
     * @param NumElements - Number of elements to remove
     */
    void RemoveAt(SizeType Position, SizeType NumElements) noexcept
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
     * @brief          - Removes the element at the position 
     * @param Position - Position of element to remove
     */
    FORCEINLINE void RemoveAt(SizeType Position) noexcept
    {
        RemoveAt(Position, 1);
    }

    /**
     * @brief         - Search the array and remove all instances of the element from the array if it is found. 
     * @param Element - Element to remove
     * @return        - Returns true if the element was found and removed, false otherwise
     */
    FORCEINLINE bool Remove(const ElementType& Element) noexcept
    {
        ElementType* Array = Allocator.GetAllocation();
        for (SizeType Index = 0; Index < ArraySize;)
        {
            if (Element == Array[Index])
            {
                RemoveAt(Index);
            }
            else
            {
                ++Index;
            }
        }

        return false;
    }

    /**
     * @brief         - Search the array and remove the all instances of the element from the array if the predicate returns true.
     * @param Element - Element to remove
     */
    template<typename PredicateType>
    FORCEINLINE void RemoveAll(PredicateType&& Predicate) noexcept
    {
        ElementType* Array = Allocator.GetAllocation();
        for (SizeType Index = 0; Index < ArraySize;)
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
     * @brief         - Returns the index of an element if it is present in the array, or -1 if it is not found
     * @param Element - Element to search for
     * @return        - The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
    {
        for (const ElementType* RESTRICT Start = Allocator.GetAllocation(), *RESTRICT Current = Start, *RESTRICT End = Start + ArraySize; Current != End; ++Current)
        {
            if (Element == *Current)
            {
                return static_cast<SizeType>(Current - Start);
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
        for (const ElementType* RESTRICT Start = Allocator.GetAllocation(), *RESTRICT Current = Start, *RESTRICT End = Start + ArraySize; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - Start);
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
        for (const ElementType* RESTRICT Start = Allocator.GetAllocation(), *RESTRICT Current = Start + ArraySize, *RESTRICT End = Start; Current != End;)
        {
            --Current;
            if (Element == *Current)
            {
                return static_cast<SizeType>(Current - Start);
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
        for (const ElementType* RESTRICT Start = Allocator.GetAllocation(), *RESTRICT Current = Start + ArraySize, *RESTRICT End = Start; Current != End;)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - Start);
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
    FORCEINLINE void Foreach(LambdaType&& Lambda) noexcept
    {
        for (ElementType* RESTRICT Current = Allocator.GetAllocation(), *RESTRICT End = Current + ArraySize; Current != End; ++Current)
        {
            Lambda(*Current);
        }
    }

    /**
     * @brief        - Perform some function on each element in the array
     * @param Lambda - Callable that takes one element and perform some operation on it
     */
    template<class LambdaType>
    FORCEINLINE void Foreach(LambdaType&& Lambda) const noexcept
    {
        for (const ElementType* RESTRICT Current = Allocator.GetAllocation(), *RESTRICT End = Current + ArraySize; Current != End; ++Current)
        {
            Lambda(*Current);
        }
    }

    /**
     * @brief             - Swap two elements with each other
     * @param FirstIndex  - Index to the first element to Swap
     * @param SecondIndex - Index to the second element to Swap
     */
    FORCEINLINE void Swap(SizeType FirstIndex, SizeType SecondIndex) noexcept
    {
        CHECK(IsValidIndex(FirstIndex));
        CHECK(IsValidIndex(SecondIndex));
        ElementType* Array = Allocator.GetAllocation();
        ::Swap(Array[FirstIndex], Array[SecondIndex]);
    }

    /**
     * @brief - Shrink the allocation to perfectly fit with the size of the array
     */
    FORCEINLINE void Shrink() noexcept
    {
        Reserve(ArraySize);
    }

    /**
     * @brief - Reverses the order for the Array
     */
    void Reverse()
    {
        const SizeType HalfSize = ArraySize / 2;
        ElementType* Array = Allocator.GetAllocation();
        for (SizeType Index = 0; Index < HalfSize; ++Index)
        {
            const SizeType ReverseIndex = ArraySize - Index - 1;
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
     * @brief - Sort the array using the quick-sort algorithm, assumes that the ElementType has the '<' operator
     */
    template<typename PredicateType>
    FORCEINLINE void SortWithPredicate(PredicateType&& Predicate)
    {
        SortInternal(0, LastElementIndex(), ::Forward(Predicate));
    }

    /**
     * @brief         - Checks that the pointer is a part of the array
     * @param Address - Address to check.
     * @return        - Returns true if the address belongs to the array
     */
    NODISCARD FORCEINLINE bool CheckAddress(const ElementType* Address) const noexcept
    {
        const ElementType* Array = Allocator.GetAllocation();
        return Address >= Array && Address < (Array + ArrayMax);
    }

    /**
     * @brief  - Checks if an index is a valid index
     * @return - Returns true if the index is valid
     */
    NODISCARD FORCEINLINE bool IsValidIndex(SizeType Index) const noexcept
    {
        return (Index >= 0) && (Index < ArraySize);
    }

    /**
     * @brief  - Check if the container contains any elements
     * @return - Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ArraySize == 0);
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE ElementType& FirstElement() noexcept
    {
        CHECK(!IsEmpty());
        ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        CHECK(!IsEmpty());
        const ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE ElementType& LastElement() noexcept
    {
        CHECK(!IsEmpty());
        ElementType* Array = Allocator.GetAllocation();
        return Array[LastElementIndex()];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const ElementType& LastElement() const noexcept
    {
        CHECK(!IsEmpty());
        const ElementType* Array = Allocator.GetAllocation();
        return Array[LastElementIndex()];
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE ElementType* Data() noexcept
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const ElementType* Data() const noexcept
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns a the index to the last element of the array
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return ArraySize ? ArraySize - 1 : 0;
    }

    /**
     * @return - Returns the size of the container
     */
    NODISCARD FORCEINLINE SizeType Size() const noexcept
    {
        return ArraySize;
    }

    /**
     * @return - Returns the stride of each element of the container
     */
    NODISCARD constexpr SizeType Stride() const noexcept
    {
        return sizeof(ElementType);
    }

    /**
     * @return - Returns the size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return ArraySize * sizeof(ElementType);
    }

    /**
     * @return - Returns the capacity of the container
     */
    NODISCARD FORCEINLINE SizeType Capacity() const noexcept
    {
        return ArrayMax;
    }

    /**
     * @return - Returns the capacity of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return ArrayMax * sizeof(ElementType);
    }

    /**
     * @return - Returns the allocator
     */
    NODISCARD FORCEINLINE AllocatorType& GetAllocator() noexcept
    {
        return Allocator;
    }

    /**
     * @return - Returns the allocator
     */
    NODISCARD FORCEINLINE const AllocatorType& GetAllocator() const noexcept
    {
        return Allocator;
    }

public: // Heap Functions

    /**
     * @brief - Create a heap of the array 
     */
    void Heapify() noexcept
    {
        const SizeType StartIndex = (ArraySize / 2) - 1;
        for (SizeType Index = StartIndex; Index >= 0; --Index)
        {
            Heapify(ArraySize, Index);
        }
    }

    /**
     * @brief  - Retrieve the top of the heap. The same as the first element.
     * @return - A reference to the element at the top of the heap
     */
    NODISCARD FORCEINLINE ElementType& HeapTop() noexcept
    {
        const ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief  - Retrieve the top of the heap. The same as the first element.
     * @return - A reference to the element at the top of the heap
     */
    NODISCARD FORCEINLINE const ElementType& HeapTop() const noexcept
    {
        const ElementType* Array = Allocator.GetAllocation();
        return Array[0];
    }

    /**
     * @brief         - Inserts a new element at the top of the heap
     * @param Element - Element to copy to the top of the heap
     */
    FORCEINLINE void HeapPush(const ElementType& Element) noexcept
    {
        Insert(0, Element);
        Heapify(ArraySize, 0);
    }

    /**
     * @brief         - Inserts a new element at the top of the heap
     * @param Element - Element to move to the top of the heap
     */
    FORCEINLINE void HeapPush(ElementType&& Element) noexcept
    {
        Insert(0, ::Forward<ElementType>(Element));
        Heapify(ArraySize, 0);
    }

    /**
     * @brief            - Remove the top of the heap and retrieve the element on top
     * @param OutElement - Reference that the top element will be copied to
     */
    FORCEINLINE void HeapPop(ElementType& OutElement) noexcept
    {
        OutElement = HeapTop();
        HeapPop();
    }

    /**
     * @brief - Remove the top of the heap
     */
    FORCEINLINE void HeapPop() noexcept
    {
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
        for (SizeType Index = ArraySize - 1; Index > 0; --Index)
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
    FORCEINLINE TArray& operator=(const TArray& Other) noexcept
    {
        Reset(Other);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - Array to move
     * @return      - A reference to this container
     */
    FORCEINLINE TArray& operator=(TArray&& Other) noexcept
    {
        InitializeByMove(::Forward<TArray>(Other));
        return *this;
    }

    /**
     * @brief       - Assignment-operator that takes a initializer-list
     * @param Other - A initializer list to replace the current contents with
     * @return      - A reference to this container
     */
    FORCEINLINE TArray& operator=(std::initializer_list<ElementType> Other) noexcept
    {
        Reset(Other);
        return *this;
    }

    /**
     * @brief       - Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param Other - Array to compare with
     * @return      - Returns true if all elements are equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator==(const ArrayType& Other) const noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        return (ArraySize == FArrayContainerHelper::Size(Other)) ? ::CompareObjects<ElementType>(Allocator.GetAllocation(), FArrayContainerHelper::Data(Other), ArraySize) : false;
    }

    /**
     * @brief       - Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param Other - Array to compare with
     * @return      - Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE bool operator!=(const ArrayType& Other) const noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        return !(*this == Other);
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SizeType Index) noexcept
    {
        ElementType* Array = Allocator.GetAllocation();
        return Array[Index];
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        const ElementType* Array = Allocator.GetAllocation();
        return Array[Index];
    }

    /**
     * @brief       - Append another array at the end of the array, which can be of another array-type
     * @param Other - Array to copy elements from
     */
    template<typename ArrayType>
    FORCEINLINE TArray& operator+=(const ArrayType& Other) noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        Append(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other));
        return *this;
    }

    /**
     * @brief          - Append an initializer-list at the end of the array, which can be of another array-type
     * @param InitList - Initializer-list to copy elements from
     */
    FORCEINLINE TArray& operator+=(std::initializer_list<ElementType> InitList) noexcept
    {
        Append(FArrayContainerHelper::Data(InitList), FArrayContainerHelper::Size(InitList));
        return *this;
    }

public:
    NODISCARD friend FORCEINLINE TArray operator+(const TArray& LHS, const TArray& RHS) noexcept
    {
        TArray NewArray(LHS, RHS.Size());
        NewArray.Append(RHS);
        return NewArray;
    }

public: // Iterators

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType Iterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

public: // STL Iterator
    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return ConstIterator(); }
    
    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return IteratorType(*this, ArraySize); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return ConstIteratorType(*this, ArraySize); }

private:
    FORCEINLINE void CreateUnitialized(SizeType NumElements)
    {
        if (ArrayMax < NumElements)
        {
            Allocator.Realloc(ArrayMax, NumElements);
            ArrayMax = NumElements;
        }

        ArraySize = NumElements;
    }

    FORCEINLINE void InitializeEmpty(SizeType NumElements)
    {
        CreateUnitialized(NumElements);
        ::DefaultConstructObjects<ElementType>(Allocator.GetAllocation(), NumElements);
    }

    FORCEINLINE void Initialize(SizeType NumElements, const ElementType& Element)
    {
        CreateUnitialized(NumElements);
        ::ConstructObjectsFrom<ElementType>(Allocator.GetAllocation(), NumElements, Element);
    }

    FORCEINLINE void InitializeByCopy(const ElementType* Elements, SizeType NumElements, SizeType ExtraCapacity)
    {
        const SizeType NewSize = NumElements + ExtraCapacity;
        CreateUnitialized(NewSize);
        ::CopyConstructObjects<ElementType>(Allocator.GetAllocation(), Elements, NumElements);
    }

    FORCEINLINE void InitializeByMove(TArray&& FromArray)
    {
        if (FromArray.Data() != Allocator.GetAllocation())
        {
            // Since the memory remains the same we should not need to use move-assignment or constructor. However, still need to call destructors
            ::DestroyObjects<ElementType>(Allocator.GetAllocation(), ArraySize);
            Allocator.MoveFrom(::Move(FromArray.Allocator));

            ArraySize           = FromArray.ArraySize;
            ArrayMax            = FromArray.ArrayMax;
            FromArray.ArraySize = 0;
            FromArray.ArrayMax  = 0;
        }
    }

    FORCEINLINE void ReserveUnchecked(const SizeType NewCapacity) noexcept
    {
        if constexpr (TNot<TIsReallocatable<ElementType>>::Value)
        {
            if (ArrayMax)
            {
                // For non-trivial objects a new allocator is necessary in order to correctly reallocate objects. This in case
                // objects has references to themselves or "child-objects" that references these objects.
                AllocatorType NewAllocator;
                NewAllocator.Realloc(ArrayMax, NewCapacity);
                if (ArraySize)
                {
                    ::RelocateObjects<ElementType>(NewAllocator.GetAllocation(), Allocator.GetAllocation(), ArraySize);
                }

                Allocator.MoveFrom(::Move(NewAllocator));
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

    FORCEINLINE void InsertUninitializedUnchecked(const SizeType Position, const SizeType InNumElements) noexcept
    {
        EnsureCapacity(ArraySize + InNumElements);
        ElementType* const CurrentAddress = Allocator.GetAllocation() + Position;
        ::RelocateObjects<ElementType>(CurrentAddress + InNumElements, CurrentAddress, ArraySize - Position);
    }

    FORCEINLINE void EnsureCapacity(SizeType RequiredCapacity) noexcept
    {
        if (RequiredCapacity > ArrayMax)
        {
            const SizeType NewCapacity = CalculateGrowth(RequiredCapacity, ArrayMax);
            ReserveUnchecked(NewCapacity);
        }
    }

    // TODO: Better to have top in back? Better to do recursive?
    FORCEINLINE void Heapify(SizeType InSize, SizeType Index) noexcept
    {
        SizeType StartIndex = Index;
        SizeType Largest    = Index;

        ElementType* Array = Allocator.GetAllocation();
        while (true)
        {
            const SizeType Left  = LeftIndex(StartIndex);
            const SizeType Right = RightIndex(StartIndex);

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

    NODISCARD static FORCEINLINE SizeType LeftIndex(SizeType Index) noexcept
    {
        return 2 * Index + 1;
    }

    NODISCARD static FORCEINLINE SizeType RightIndex(SizeType Index) noexcept
    {
        return 2 * Index + 2;
    }

    // Calculate how much the array should grow, will always be at least one
    NODISCARD static FORCEINLINE SizeType CalculateGrowth(SizeType NumElements, SizeType CurrentCapacity) noexcept
    {
        constexpr SizeType FirstAlloc = 4;

        SizeType NewSize;
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

    template<typename PredicateType>
    FORCEINLINE SizeType SortPartition(SizeType First, SizeType Last, PredicateType&& Predicate) noexcept
    {
        // Select a random pivot value
        ElementType* Array = Allocator.GetAllocation();
        const SizeType Pivot = Last;
        SizeType Index = First;
        for (SizeType Current = First; Current < Last; ++Current)
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
    void SortInternal(SizeType First, SizeType Last, PredicateType&& Predicate) noexcept
    {
        if (First >= Last)
        {
            return;
        }

        // Max number of elements to use for QuickSort, otherwise use InsertionSort
        constexpr SizeType Threshold = 24;
        if ((Last - First + 1) >= Threshold)
        {
            const SizeType Pivot = SortPartition(First, Last, Predicate);
            SortInternal(First    , Pivot - 1, Predicate);
            SortInternal(Pivot + 1, Last     , Predicate);
        }
        else
        {
            ElementType* Array = Allocator.GetAllocation();
            for (SizeType Current = First + 1; Current <= Last; ++Current) 
            {
                SizeType Index = Current;
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
    SizeType      ArraySize = 0;
    SizeType      ArrayMax  = 0;
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
