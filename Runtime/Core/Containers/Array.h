#pragma once
#include "Allocators.h"
#include "UniquePtr.h"
#include "ArrayView.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/Functional.h"

template<typename T, typename AllocatorType = TDefaultArrayAllocator<T>>
class TArray
{
public:
    using ElementType = T;
    using SizeType    = int32;

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

    /**
     * @brief - Default constructor
     */
    FORCEINLINE TArray() noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
    }

    /** 
     * @brief        - Constructor that default creates a certain number of elements 
     * @param InSize - Number of elements to construct
     */
    FORCEINLINE explicit TArray(SizeType InSize) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
        InitializeEmpty(InSize);
    }

    /**
     * @brief         - Constructor that Allocates the specified amount of elements, and initializes them to the same value 
     * @param InSize  - Number of elements to construct
     * @param Element - Element to copy into all positions of the array
     */
    FORCEINLINE TArray(SizeType InSize, const ElementType& Element) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
        Initialize(InSize, Element);
    }

    /**
     * @brief               - Constructor that creates an array from a raw pointer array 
     * @param InputArray    - Pointer to the start of the array to copy from
     * @param InNumElements - Number of elements in 'InputArray', which also is the resulting size of the constructed array
     */
    FORCEINLINE TArray(const ElementType* InputArray, SizeType InNumElements) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
        InitializeByCopy(InputArray, InNumElements, 0);
    }

    /** 
     * @brief       - Copy-constructor
     * @param Other - Array to copy from
     */
    FORCEINLINE TArray(const TArray& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
        InitializeByCopy(Other.Data(), Other.Size(), 0);
    }

    /**
     * @brief       - Copy-constructor from another type of array
     * @param Other - Array to copy from
     */
    template<
        typename ArrayType,
        typename = typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type>
    FORCEINLINE explicit TArray(const ArrayType& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
        InitializeByCopy(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other), 0);
    }

    /** 
     * @brief       - Move-constructor 
     * @param Other - Array to move elements from
     */
    FORCEINLINE TArray(TArray&& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
        InitializeByMove(::Forward<TArray>(Other));
    }

    /** 
     * @brief        - Constructor that creates an array from an std::initializer_list
     * @param InList - Initializer list containing all elements to construct the array from
     */
    FORCEINLINE TArray(std::initializer_list<ElementType> InList) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayMax(0)
    {
        InitializeByCopy(FArrayContainerHelper::Data(InList), FArrayContainerHelper::Size(InList), 0);
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
            ::DestroyElements<ElementType>(Allocator.Data(), ArraySize);
            ArraySize = 0;
        }

        if (bRemoveSlack)
        {
            Allocator.Free();
            ArrayMax = 0;
        }
    }

    /**
     * @brief - Resets the container, but does not deallocate the memory. Takes an optional parameter to 
     *    default construct a new amount of elements.
     * 
     * @param NewSize - Number of elements to construct
     */
    void Reset(SizeType InSize = 0) noexcept
    {
        ::DestroyElements<ElementType>(Allocator.Data(), ArraySize);
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
     * @brief - Resets the container, but does not deallocate the memory. Takes an optional parameter to
     *     default construct a new amount of elements from a single element.
     * 
     * @param NewSize - Number of elements to construct
     * @param Element - Element to copy-construct from
     */
    void Reset(SizeType NewSize, const ElementType& Element) noexcept
    {
        ::DestroyElements<ElementType>(Allocator.Data(), ArraySize);
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
     * @brief - Resets the container, but does not deallocate the memory. Takes in pointer to copy-construct elements from.
     * 
     * @param Elements      - Array to copy-construct from
     * @param InNumElements - Number of elements from
     */
    void Reset(const ElementType* Elements, SizeType NumElements) noexcept
    {
        if (NumElements <= 0)
        {
            return;
        }

        if (Elements != Allocator.Data())
        {
            ::DestroyElements<ElementType>(Allocator.Data(), ArraySize);
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
     * @brief - Resets the container, but does not deallocate the memory. Creates a new array from
     *     another array which can be of another type of array.
     * 
     * @param InputArray - Array to copy-construct from
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Reset(const ArrayType& InputArray) noexcept
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
     * @brief        - Resets the container and copy-construct a new array from an initializer-list
     * @param InList - Initializer-list to copy-construct elements from
     */
    FORCEINLINE void Reset(std::initializer_list<ElementType> InList) noexcept
    {
        Reset(FArrayContainerHelper::Data(InList), FArrayContainerHelper::Size(InList));
    }

    /** 
     * @brief              - Fill the container with the specified value 
     * @param InputElement - Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ::AssignElements(Allocator.Data(), InputElement, ArraySize);
    }

    /**
     * @brief         - Resizes the container with a new size, and default constructs them
     * @param NewSize - The new size of the array
     */
    void Resize(SizeType NewSize) noexcept
    {
        if (NewSize > ArraySize)
        {
            EnsureCapacity(NewSize);

            // NewSize is always larger than array-size...
            const SizeType NumElementsToConstruct = NewSize - ArraySize;
            // ...However, assert just in case
            CHECK(NumElementsToConstruct > 0);

            ElementType* LastElementPtr = Allocator.Data() + ArraySize;
            ::DefaultConstructElements<ElementType>(LastElementPtr, NumElementsToConstruct);
            ArraySize = NewSize;
        }
        else if (NewSize < ArraySize)
        {
            const SizeType NumElementsToDestruct = ArraySize - NewSize;
            CHECK(NumElementsToDestruct > 0);
            PopRangeUnchecked(NumElementsToDestruct);
        }
    }

    /**
     * @brief          - Resizes the container with a new size, and constructs them with value 
     * @param NewSize  - The new size of the array
     * @param Elements - Element to copy into all positions of the array
     */
    void Resize(SizeType NewSize, const ElementType& Element) noexcept
    {
        if (NewSize > ArraySize)
        {
            EnsureCapacity(NewSize);

            // NewSize is always larger than arraysize...
            const SizeType NumElementsToConstruct = NewSize - ArraySize;
            // ...However, assert just in case
            CHECK(NumElementsToConstruct > 0);

            ElementType* TmpLastElement = Allocator.Data() + ArraySize;
            ::ConstructElementsFrom<ElementType>(TmpLastElement, NumElementsToConstruct, Element);
            ArraySize = NewSize;
        }
        else if (NewSize < ArraySize)
        {
            const SizeType NumElementsToDestruct = ArraySize - NewSize;
            CHECK(NumElementsToDestruct > 0);
            PopRangeUnchecked(NumElementsToDestruct);
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
                ::DestroyElements<ElementType>(Allocator.Data() + NewCapacity, ArraySize - NewCapacity);
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
        new(reinterpret_cast<void*>(Allocator.Data() + (ArraySize++))) ElementType(::Forward<ArgTypes>(Args)...);
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
        return (ArraySize - 1);
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
        return (ArraySize - 1);
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
        new(reinterpret_cast<void*>(Allocator.Data() + Position)) ElementType(::Forward<ArgTypes>(Args)...);
        ArraySize++;
    }

    /**
     * @brief          - Constructs a new element at a specific position in the array
     * @param Position - Iterator pointing to the position of the new element
     * @param Args     - Arguments for the constructor of the element
     */
    template<typename... ArgTypes>
    FORCEINLINE void EmplaceAt(ConstIteratorType Position, ArgTypes&&... Args) noexcept
    {
        EmplaceAt(Position.GetIndex(), ::Forward<ArgTypes>(Args)...);
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
     * @brief          - Insert a new element at a specific position in the array
     * @param Position - Iterator pointing to the position of the new element
     * @param Element  - Element to copy into the position
     */
    FORCEINLINE void Insert(ConstIteratorType Position, const ElementType& Element) noexcept
    {
        EmplaceAt(Position.GetIndex(), Element);
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
     * @brief          - Insert a new element at the a specific position in the array by moving
     * @param Position - Iterator pointing to the position of the new element
     * @param Element  - Element to move into the position
     */
    FORCEINLINE void Insert(ConstIteratorType Position, ElementType&& Element) noexcept
    {
        EmplaceAt(Position.GetIndex(), ::Forward<ElementType>(Element));
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
        ::CopyConstructElements<ElementType>(Allocator.Data() + Position, Elements, InNumElements);
        ArraySize += InNumElements;
    }

    /**
     * @brief             - Insert an array at a specific position in the array
     * @param Position    - Iterator pointing to the position of the new element
     * @param InputArray  - Array to copy into the array
     * @param NumElements - Number of elements in the input-array
     */
    FORCEINLINE void Insert(ConstIteratorType Position, const ElementType* InputArray, SizeType NumElements) noexcept
    {
        Insert(Position.GetIndex(), InputArray, NumElements);
    }

    /**
     * @brief          - Insert elements from a initializer list at a specific position in the array
     * @param Position - Position of the new element
     * @param InList   - Initializer list to insert into the array
     */
    FORCEINLINE void Insert(SizeType Position, std::initializer_list<ElementType> InList) noexcept
    {
        Insert(Position, FArrayContainerHelper::Data(InList), FArrayContainerHelper::Size(InList));
    }

    /**
     * @brief          - Insert elements from a initializer list at a specific position in the array
     * @param Position - Iterator pointing to the position of the new element
     * @param InList   - Initializer list to insert into the array
     */
    FORCEINLINE void Insert(ConstIteratorType Position, std::initializer_list<ElementType> InList) noexcept
    {
        Insert(Position.GetIndex(), FArrayContainerHelper::Data(InList), FArrayContainerHelper::Size(InList));
    }

    /**
      * @brief - Insert elements from another array at a specific position in the array, which
      *     can be of another array-type.
      * 
      * @param Position - Position of the new element
      * @param InArray  - Array to copy elements from
      */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Insert(SizeType Position, const ArrayType& InArray) noexcept
    {
        Insert(Position, FArrayContainerHelper::Data(InArray), FArrayContainerHelper::Size(InArray));
    }

    /**
      * @brief - Insert elements from another array at a specific position in the array, which
      *     can be of another array-type.
      * 
      * @param Position - Iterator pointing to the position of the new element
      * @param InArray  - Array to copy elements from
      */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Insert(ConstIteratorType Position, const ArrayType& InArray) noexcept
    {
        Insert(Position.GetIndex(), FArrayContainerHelper::Data(InArray), FArrayContainerHelper::Size(InArray));
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
     * @param InputArray  - Array to copy elements from
     * @param NumElements - Number of elements in the input-array
     */
    void Append(const ElementType* InputArray, SizeType NumElements) noexcept
    {
        if (NumElements > 0)
        {
            CHECK(InputArray != nullptr);
            EnsureCapacity(ArraySize + NumElements);
            ::CopyConstructElements<ElementType>(Allocator.Data() + ArraySize, InputArray, NumElements);
            ArraySize += NumElements;
        }
    }

    /**
     * @brief       - Insert another array at the end of the array, which can be of another array-type
     * @param Other - Array to copy elements from
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Append(const ArrayType& Other) noexcept
    {
        Append(FArrayContainerHelper::Data(Other), FArrayContainerHelper::Size(Other));
    }

    /**
     * @brief        - Insert an initializer-list at the end of the array, which can be of another array-type
     * @param InList - Initializer-list to copy elements from
     */
    FORCEINLINE void Append(std::initializer_list<ElementType> InList) noexcept
    {
        Append(FArrayContainerHelper::Data(InList), FArrayContainerHelper::Size(InList));
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
    FORCEINLINE void PopRange(SizeType NumElements) noexcept
    {
        if (!IsEmpty())
        {
            PopRangeUnchecked(NumElements);
        }
    }

    /** 
     * @brief - Remove the last element 
     */
    FORCEINLINE void Pop() noexcept
    {
        PopRange(1);
    }

    /**
     * @brief             - Remove a range of elements starting at position
     * @param Position    - Position of the array to start remove elements from
     * @param NumElements - Number of elements to remove
     */
    void RemoveRangeAt(SizeType Position, SizeType NumElements) noexcept
    {
        CHECK(Position + NumElements <= ArraySize);

        if (NumElements)
        {
            ElementType* TmpPositionData = Allocator.Data() + Position;
            ::DestroyElements<ElementType>(TmpPositionData, NumElements);
            ::RelocateElements<ElementType>(TmpPositionData, TmpPositionData + NumElements, ArraySize - (Position + NumElements));
            ArraySize -= NumElements;
        }
    }

    /**
     * @brief          - Removes the element at the position 
     * @param Position - Position of element to remove
     */
    FORCEINLINE void RemoveAt(SizeType Position) noexcept
    {
        RemoveRangeAt(Position, 1);
    }

    /**
     * @brief          - Removes the element at the position
     * @param Position - Iterator pointing to the position of element to remove
     * @return         - Returns the iterator to the element that follows the removed element
     */
    FORCEINLINE IteratorType RemoveAt(IteratorType Iterator) noexcept
    {
        CHECK(Iterator.IsFrom(*this));
        RemoveAt(Iterator.GetIndex());
        return Iterator;
    }

    /**
     * @brief          - Removes the element at the position
     * @param Position - Iterator pointing to the position of element to remove
     * @return         - Returns the iterator to the element that follow the removed element
     */
    FORCEINLINE ConstIteratorType RemoveAt(ConstIteratorType Iterator) noexcept
    {
        CHECK(Iterator.IsFrom(*this));
        RemoveAt(Iterator.GetIndex());
        return Iterator;
    }

    /**
     * @brief         - Search the array and remove the first instance of the element from the array if it is found. 
     * @param Element - Element to remove
     * @return        - Returns true if the element was found and removed, false otherwise
     */
    FORCEINLINE bool Remove(const ElementType& Element) noexcept
    {
        for(SizeType Index = 0; Index < ArraySize; ++Index)
        {
            if (Element == Allocator[Index])
            {
                RemoveAt(Index);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief - Search the array and remove the first instance of 
     *     the element from the array if the predicate returns true.
     * 
     * @param Element - Element to remove
     */
    template<typename PredicateType>
    FORCEINLINE void RemoveWithPredicate(PredicateType&& Predicate) noexcept
    {
        for (SizeType Index = 0; Index < ArraySize; ++Index)
        {
            if (Predicate(Allocator[Index]))
            {
                RemoveAt(Index);
                break;
            }
        }
    }

    /**
     * @brief         - Search the array and remove the all instances of the element from the array if it is found.
     * @param Element - Element to remove
     */
    FORCEINLINE void RemoveAll(const ElementType& Element) noexcept
    {
        for (SizeType Index = 0; Index < ArraySize;)
        {
            if (Element == Allocator[Index])
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
     * @brief - Search the array and remove the all instances of the 
     *     element from the array if the predicate returns true.
     * 
     * @param Element - Element to remove
     */
    template<typename PredicateType>
    FORCEINLINE void RemoveAllWithPredicate(PredicateType&& Predicate) noexcept
    {
        for (SizeType Index = 0; Index < ArraySize;)
        {
            if (Predicate(Allocator[Index]))
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
        const ElementType* RESTRICT CurrentAddress = Allocator.Data();
        const ElementType* RESTRICT EndAddress     = Allocator.Data() + ArraySize;
        while (CurrentAddress != EndAddress)
        {
            if (Element == *CurrentAddress)
            {
                return static_cast<SizeType>(CurrentAddress - Allocator.Data());
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
        const ElementType* RESTRICT CurrentAddress = Allocator.Data();
        const ElementType* RESTRICT EndAddress     = Allocator.Data() + ArraySize;
        while (CurrentAddress != EndAddress)
        {
            if (Predicate(*CurrentAddress))
            {
                return static_cast<SizeType>(CurrentAddress - Allocator.Data());
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
        const ElementType* RESTRICT CurrentAddress = Allocator.Data() + ArraySize;
        const ElementType* RESTRICT EndAddress     = Allocator.Data();
        while (CurrentAddress != EndAddress)
        {
            --CurrentAddress;
            if (Element == *CurrentAddress)
            {
                return static_cast<SizeType>(CurrentAddress - Allocator.Data());
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
        const ElementType* RESTRICT CurrentAddress = Allocator.Data() + ArraySize;
        const ElementType* RESTRICT EndAddress     = Allocator.Data();
        while (CurrentAddress != EndAddress)
        {
            --CurrentAddress;
            if (Predicate(*CurrentAddress))
            {
                return static_cast<SizeType>(CurrentAddress - Allocator.Data());
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
        return (FindWithPredicate(::Forward<PredicateType>(Predicate)) != INVALID_INDEX);
    }

    /**
     * @brief         - Perform some function on each element in the array
     * @param Functor - Callable that takes one element and perform some operation on it
     */
    template<class FunctorType>
    FORCEINLINE void Foreach(FunctorType&& Functor)
    {
        ElementType* RESTRICT CurrentAddress = Allocator.Data();
        ElementType* RESTRICT EndAddress     = Allocator.Data() + ArraySize;
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
    FORCEINLINE void Swap(TArray& Other) noexcept
    {
        TArray TempArray(::Move(*this));
        InitializeByMove(::Move(Other));
        Other.InitializeByMove(::Move(TempArray));
    }

    /**
     * @brief - Shrink the allocation to perfectly fit with the size of the array
     */
    FORCEINLINE void Shrink() noexcept
    {
        Reserve(ArraySize);
    }

    /**
     * @brief         - Checks that the pointer is a part of the array
     * @param Address - Address to check.
     * @return        - Returns true if the address belongs to the array
     */
    NODISCARD FORCEINLINE bool CheckAddress(const ElementType* Address) const noexcept
    {
        return (Address >= Allocator.Data()) && (Address < (Allocator.Data() + ArrayMax));
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
        return Allocator[0];
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        CHECK(!IsEmpty());
        return Allocator[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE ElementType& LastElement() noexcept
    {
        CHECK(!IsEmpty());
        const int32 LastIndex = ArraySize - 1; 
        return Allocator[LastIndex];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const ElementType& LastElement() const noexcept
    {
        CHECK(!IsEmpty());
        const int32 LastIndex = ArraySize - 1; 
        return Allocator[LastIndex];
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE ElementType* Data() noexcept
    {
        return Allocator.Data();
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const ElementType* Data() const noexcept
    {
        return Allocator.Data();
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns a the index to the last element of the array
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (ArraySize > 0) ? (ArraySize - 1) : 0;
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
    NODISCARD CONSTEXPR SizeType Stride() const noexcept
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

public:

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
        return Allocator[0];
    }

    /**
     * @brief  - Retrieve the top of the heap. The same as the first element.
     * @return - A reference to the element at the top of the heap
     */
    NODISCARD FORCEINLINE const ElementType& HeapTop() const noexcept
    {
        return Allocator[0];
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
        for (SizeType Index = ArraySize - 1; Index > 0; --Index)
        {
            ::Swap<ElementType>(Allocator[0], Allocator[Index]);
            Heapify(Index, 0);
        }
    }

    /**
     * @brief - Reverses the order for the Array
     */
    void Reverse()
    {
        const int32 HalfSize = ArraySize / 2;
        for (SizeType Index = 0; Index < HalfSize; ++Index)
        {
            const int32 ReverseIndex = ArraySize - Index - 1;
            ::Swap(Allocator[Index], Allocator[ReverseIndex]);
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
    NODISCARD FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator==(const ArrayType& Other) const noexcept
    {
        return (ArraySize == Other.ArraySize) ? ::CompareElements<ElementType>(Allocator.Data(), FArrayContainerHelper::Data(Other), ArraySize) : (false);
    }

    /**
     * @brief       - Comparison operator that compares all elements in the array, which can be of any ArrayType qualified type
     * @param Other - Array to compare with
     * @return      - Returns true if all elements are NOT equal to each other
     */
    template<typename ArrayType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value, bool>::Type operator!=(const ArrayType& Other) const noexcept
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
        return Allocator[Index];
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        return Allocator[Index];
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
        return IteratorType(*this, Size());
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
        return ConstIteratorType(*this, Size());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
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
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
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
        ::DefaultConstructElements<ElementType>(Allocator.Data(), NumElements);
    }

    FORCEINLINE void Initialize(SizeType NumElements, const ElementType& Element)
    {
        CreateUnitialized(NumElements);
        ::ConstructElementsFrom<ElementType>(Allocator.Data(), NumElements, Element);
    }

    FORCEINLINE void InitializeByCopy(const ElementType* Elements, SizeType NumElements, SizeType ExtraCapacity)
    {
        const SizeType NewSize = NumElements + ExtraCapacity;
        CreateUnitialized(NewSize);
        ::CopyConstructElements<ElementType>(Allocator.Data(), Elements, NumElements);
    }

    FORCEINLINE void InitializeByMove(TArray&& FromArray)
    {
        if (FromArray.Data() != Allocator.Data())
        {
            // Since the memory remains the same we should not need to use move-assignment or constructor. However, still need to call destructors
            ::DestroyElements<ElementType>(Allocator.Data(), ArraySize);
            Allocator.MoveFrom(::Move(FromArray.Allocator));

            ArraySize           = FromArray.ArraySize;
            ArrayMax            = FromArray.ArrayMax;
            FromArray.ArraySize = 0;
            FromArray.ArrayMax  = 0;
        }
    }

    FORCEINLINE void ReserveUnchecked(const SizeType NewCapacity) noexcept
    {
        if CONSTEXPR (TNot<TIsReallocatable<ElementType>>::Value)
        {
            if (ArrayMax)
            {
                // For non-trivial objects a new allocator is necessary in order to correctly reallocate objects. This in case
                // objects has references to themselves or "child-objects" that references these objects.
                AllocatorType NewAllocator;
                NewAllocator.Realloc(ArrayMax, NewCapacity);
                if (ArraySize)
                {
                    ::RelocateElements<ElementType>(NewAllocator.Data(), Allocator.Data(), ArraySize);
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
        ElementType* const CurrentAddress = Allocator.Data() + Position;
        ::RelocateElements<ElementType>(CurrentAddress + InNumElements, CurrentAddress, ArraySize - Position);
    }

    FORCEINLINE void PopRangeUnchecked(SizeType InNumElements) noexcept
    {
        const SizeType NewArraySize = ArraySize - InNumElements;
        ::DestroyElements<ElementType>(Allocator.Data() + NewArraySize, InNumElements);
        ArraySize = NewArraySize;
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

        while (true)
        {
            const SizeType Left  = LeftIndex(StartIndex);
            const SizeType Right = RightIndex(StartIndex);

            if (Left < InSize && Allocator[Left] > Allocator[Largest])
            {
                Largest = Left;
            }

            if (Right < InSize && Allocator[Right] > Allocator[Largest])
            {
                Largest = Right;
            }

            if (Largest != StartIndex)
            {
                ::Swap<ElementType>(Allocator[StartIndex], Allocator[Largest]);
                StartIndex = Largest;
            }
            else
            {
                break;
            }
        }
    }

    NODISCARD static FORCEINLINE SizeType LeftIndex(SizeType Index)
    {
        return (2 * Index + 1);
    }

    NODISCARD static FORCEINLINE SizeType RightIndex(SizeType Index)
    {
        return (2 * Index + 2);
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

private:
    AllocatorType Allocator;
    SizeType      ArraySize;
    SizeType      ArrayMax;
};

template<typename T, typename AllocatorType>
struct TIsTArrayType<TArray<T, AllocatorType>>
{
    enum { Value = true };
};

template<typename T, typename AllocatorType>
struct TIsContiguousContainer<TArray<T, AllocatorType>>
{
    enum { Value = true };
};
