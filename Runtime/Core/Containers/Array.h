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

    enum : SizeType { INVALID_INDEX = -1 };

public:

    /** 
     * @brief - Default constructor
     */
    FORCEINLINE TArray() noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    { }

    /** 
     * @brief        - Constructor that default creates a certain number of elements 
     * @param InSize - Number of elements to construct
     */
    FORCEINLINE explicit TArray(SizeType InSize) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        ConstructEmpty(InSize);
    }

    /**
     * @brief         - Constructor that Allocates the specified amount of elements, and initializes them to the same value 
     * @param InSize  - Number of elements to construct
     * @param Element - Element to copy into all positions of the array
     */
    FORCEINLINE TArray(SizeType InSize, const ElementType& Element) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        ConstructFrom(InSize, Element);
    }

    /**
     * @brief             - Constructor that creates an array from a raw pointer array 
     * @param InputArray  - Pointer to the start of the array to copy from
     * @param NumElements - Number of elements in 'InputArray', which also is the resulting size of the constructed array
     */
    FORCEINLINE TArray(const ElementType* InputArray, SizeType NumElements) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(InputArray, NumElements, 0);
    }

    /** 
     * @brief       - Copy-constructor
     * @param Other - Array to copy from
     */
    FORCEINLINE TArray(const TArray& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(Other.GetData(), Other.GetSize(), 0);
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
        , ArrayCapacity(0)
    {
        CopyConstructFrom(FContiguousContainerHelper::GetData(Other), FContiguousContainerHelper::GetSize(Other), 0);
    }

    /** 
     * @brief       - Move-constructor 
     * @param Other - Array to move elements from
     */
    FORCEINLINE TArray(TArray&& Other) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        MoveFrom(Forward<TArray>(Other));
    }

    /** 
     * @brief        - Constructor that creates an array from an std::initializer_list
     * @param InList - Initializer list containing all elements to construct the array from
     */
    FORCEINLINE TArray(std::initializer_list<ElementType> InList) noexcept
        : Allocator()
        , ArraySize(0)
        , ArrayCapacity(0)
    {
        CopyConstructFrom(FContiguousContainerHelper::GetData(InList), FContiguousContainerHelper::GetSize(InList), 0);
    }

    /** 
     * @brief - Destructor
     */
    FORCEINLINE ~TArray()
    {
        Clear(true);
    }

    /** 
     * @brief             - Clear all elements of the container, but does not deallocate the memory
     * @param bFreeMemory - Should the memory be deallocated or not
     */
    FORCEINLINE void Clear(bool bFreeMemory = false) noexcept
    {
        if (ArraySize)
        {
            ::DestroyElements<ElementType>(GetData(), ArraySize);
            ArraySize = 0;
        }

        if (bFreeMemory)
        {
            Allocator.Free();
            ArrayCapacity = 0;
        }
    }

    /**
     * @brief - Resets the container, but does not deallocate the memory. Takes an optional parameter to 
     *    default construct a new amount of elements.
     * 
     * @param NewSize - Number of elements to construct
     */
    FORCEINLINE void Reset(SizeType NewSize = 0) noexcept
    {
        ::DestroyElements<ElementType>(GetData(), ArraySize);
        if (NewSize)
        {
            ConstructEmpty(NewSize);
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
    FORCEINLINE void Reset(SizeType NewSize, const ElementType& Element) noexcept
    {
        ::DestroyElements<ElementType>(GetData(), ArraySize);
        if (NewSize)
        {
            ConstructFrom(NewSize, Element);
        }
        else
        {
            ArraySize = 0;
        }
    }

    /** 
     * @brief - Resets the container, but does not deallocate the memory. Takes in pointer to copy-construct elements from.
     * 
     * @param InputArray  - Array to copy-construct from
     * @param NumElements - Number of elements from
     */
    FORCEINLINE void Reset(const ElementType* InputArray, SizeType NumElements) noexcept
    {
        if (NumElements > 0)
        {
            CHECK(InputArray != nullptr);
            if (InputArray != GetData())
            {
                ::DestroyElements<ElementType>(GetData(), ArraySize);
                if (NumElements)
                {
                    CopyConstructFrom(InputArray, NumElements, 0);
                }
                else
                {
                    ArraySize = 0;
                }
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
        Reset(InputArray.GetData(), InputArray.GetSize());
    }

    /**
     * @brief            - Resets the container by moving elements from another array to this one.
     * @param InputArray - Array to copy-construct elements from
     */
    FORCEINLINE void Reset(TArray&& InputArray) noexcept
    {
        MoveFrom(Forward<TArray>(InputArray));
    }

    /** 
     * @brief        - Resets the container and copy-construct a new array from an initializer-list
     * @param InList - Initializer-list to copy-construct elements from
     */
    FORCEINLINE void Reset(std::initializer_list<ElementType> InList) noexcept
    {
        Reset(FContiguousContainerHelper::GetData(InList), FContiguousContainerHelper::GetSize(InList));
    }

    /** 
     * @brief              - Fill the container with the specified value 
     * @param InputElement - Element to copy into all elements in the array
     */
    FORCEINLINE void Fill(const ElementType& InputElement) noexcept
    {
        ::AssignElements(GetData(), InputElement, GetSize());
    }

    /**
     * @brief         - Resizes the container with a new size, and default constructs them
     * @param NewSize - The new size of the array
     */
    void Resize(SizeType NewSize) noexcept
    {
        if (NewSize > ArraySize)
        {
            if (NewSize >= ArrayCapacity)
            {
                ReserveUnchecked(NewSize);
            }

            // NewSize is always larger than array-size...
            const SizeType NumElementsToConstruct = NewSize - ArraySize;
            // ...However, assert just in case
            CHECK(NumElementsToConstruct > 0);

            ElementType* LastElementPtr = GetData() + ArraySize;
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
            if (NewSize >= ArrayCapacity)
            {
                ReserveUnchecked(NewSize);
            }

            // NewSize is always larger than arraysize...
            const SizeType NumElementsToConstruct = NewSize - ArraySize;
            // ...However, assert just in case
            CHECK(NumElementsToConstruct > 0);

            ElementType* TmpLastElement = GetData() + ArraySize;
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
        if (NewCapacity != ArrayCapacity)
        {
            if (NewCapacity < ArraySize)
            {
                ::DestroyElements<ElementType>(GetData() + NewCapacity, ArraySize - NewCapacity);
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
        ExpandStorage();
        new(GetData() + (ArraySize++)) ElementType(Forward<ArgTypes>(Args)...);
        return LastElement();
    }

    /**
     * @brief         - Appends a new element at the end of the array
     * @param Element - Element to insert into the array by copy
     * @return        - Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Push(const ElementType& Element) noexcept
    {
        return Emplace(Element);
    }

    /**
     * @brief         - Appends a new element at the end of the array
     * @param Element - Element to insert into the array by move
     * @return        - Returns a reference to the newly created element
     */
    FORCEINLINE ElementType& Push(ElementType&& Element) noexcept
    {
        return Emplace(Forward<ElementType>(Element));
    }

    /**
     * @brief         - Appends a new element at the end of the array if the element does not already exist
     * @param Element - Element to insert into the array by copy
     * @return        - Returns a reference to the newly created element or element equal to Element
     */
    FORCEINLINE SizeType PushUnique(const ElementType& Element) noexcept
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
    FORCEINLINE SizeType PushUnique(ElementType&& Element) noexcept
    {
        const SizeType Index = Find(Element);
        if (Index >= 0)
        {
            return Index;
        }

        Emplace(Forward<ElementType>(Element));
        return (ArraySize - 1);
    }

    /**
     * @brief - Appends a new element at the end of the array, but does not call any constructor
     */
    FORCEINLINE void PushUninitialized() noexcept
    {
        AppendUninitialized(1);
    }

    /**
     * @brief          - Constructs a new element at a specific position in the array
     * @param Position - Position of the new element
     * @param Args     - Arguments for the constructor of the element
     */
    template<typename... ArgTypes>
    FORCEINLINE void EmplaceAt(SizeType Position, ArgTypes&&... Args) noexcept
    {
        CHECK(Position <= ArraySize);
        InsertUninitializedUnchecked(Position, 1);
        new(GetData() + Position) ElementType(Forward<ArgTypes>(Args)...);
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
        EmplaceAt(Position.GetIndex(), Forward<ArgTypes>(Args)...);
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
        EmplaceAt(Position, Forward<ElementType>(Element));
    }

    /**
     * @brief          - Insert a new element at the a specific position in the array by moving
     * @param Position - Iterator pointing to the position of the new element
     * @param Element  - Element to move into the position
     */
    FORCEINLINE void Insert(ConstIteratorType Position, ElementType&& Element) noexcept
    {
        EmplaceAt(Position.GetIndex(), Forward<ElementType>(Element));
    }

    /**
     * @brief             - Insert an array at a specific position in the array
     * @param Position    - Position of the new element
     * @param InputArray  - Array to copy into the array
     * @param NumElements - Number of elements in the input-array
     */
    FORCEINLINE void Insert(SizeType Position, const ElementType* InputArray, SizeType NumElements) noexcept
    {
        CHECK(Position <= ArraySize);
        CHECK(InputArray != nullptr);

        InsertUninitializedUnchecked(Position, NumElements);
        ::CopyConstructElements<ElementType>(GetData() + Position, InputArray, NumElements);
        ArraySize += NumElements;
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
        Insert(Position, FContiguousContainerHelper::GetData(InList), FContiguousContainerHelper::GetSize(InList));
    }

    /**
     * @brief          - Insert elements from a initializer list at a specific position in the array
     * @param Position - Iterator pointing to the position of the new element
     * @param InList   - Initializer list to insert into the array
     */
    FORCEINLINE void Insert(ConstIteratorType Position, std::initializer_list<ElementType> InList) noexcept
    {
        Insert(Position.GetIndex(), FContiguousContainerHelper::GetData(InList), FContiguousContainerHelper::GetSize(InList));
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
        Insert(Position, InArray.GetData(), InArray.GetSize());
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
        Insert(Position.GetIndex(), InArray.GetData(), InArray.GetSize());
    }

    /**
     * @brief             - Insert a number of uninitialized elements in the array at a specific position
     * @param Position    - Start-position of the new elements
     * @param NumElements - Number of elements to insert
     */
    FORCEINLINE void InsertUninitialized(SizeType Position, SizeType NumElements) noexcept
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
            ExpandStorage(NumElements);
            ::CopyConstructElements<ElementType>(GetData() + ArraySize, InputArray, NumElements);
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
        Append(Other.GetData(), Other.GetSize());
    }

    /**
     * @brief        - Insert an initializer-list at the end of the array, which can be of another array-type
     * @param InList - Initializer-list to copy elements from
     */
    FORCEINLINE void Append(std::initializer_list<ElementType> InList) noexcept
    {
        Append(FContiguousContainerHelper::GetData(InList), FContiguousContainerHelper::GetSize(InList));
    }

    /**
     * @brief         - Create a number of uninitialized elements at the end of the array
     * @param NewSize - Number of elements to append
     */
    FORCEINLINE void AppendUninitialized(SizeType NumElements) noexcept
    {
        ExpandStorage(NumElements);
        ArraySize += NumElements;
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
            ::DestroyElements<ElementType>(GetData() + Position, NumElements);
            ::RelocateElements<ElementType>(GetData() + Position, GetData() + Position + NumElements, ArraySize - (Position + NumElements));
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
     * @return        - Returns true if the element was found and remved, false otherwise
     */
    FORCEINLINE bool Remove(const ElementType& Element) noexcept
    {
        for(SizeType Index = 0; Index < ArraySize; ++Index)
        {
            if (Element == GetElementAt(Index))
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
            if (Predicate(GetElementAt(Index)))
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
            if (Element == GetElementAt(Index))
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
            if (Predicate(GetElementAt(Index)))
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
        const ElementType* RESTRICT CurrentAddress = GetData();
        const ElementType* RESTRICT EndAddress     = GetData() + ArraySize;
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
     * @brief           - Returns the index of the element that satisfy the conditions of a comparator
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SizeType FindWithPredicate(PredicateType&& Predicate) const noexcept
    {
        const ElementType* RESTRICT CurrentAddress = GetData();
        const ElementType* RESTRICT EndAddress     = GetData() + ArraySize;
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
     * @brief         - Returns the index of an element if it is present in the array, or -1 if it is not found
     * @param Element - Element to search for
     * @return        - The index of the element if found or -1 if not
     */
    NODISCARD FORCEINLINE SizeType FindLast(const ElementType& Element) const noexcept
    {
        const ElementType* RESTRICT CurrentAddress = GetData() + ArraySize;
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
     * @brief           - Returns the index of the element that satisfy the conditions of a comparator
     * @param Predicate - Callable that compares an element in the array against some condition
     * @return          - The index of the element if found or INVALID_INDEX if not
     */
    template<class PredicateType>
    NODISCARD FORCEINLINE SizeType FindLastWithPredicate(PredicateType&& Predicate) const noexcept
    {
        const ElementType* RESTRICT CurrentAddress = GetData() + ArraySize;
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
        return (FindWithPredicate(Forward<PredicateType>(Predicate)) != INVALID_INDEX);
    }

    /**
     * @brief         - Perform some function on each element in the array
     * @param Functor - Callable that takes one element and perform some operation on it
     */
    template<class FunctorType>
    FORCEINLINE void Foreach(FunctorType&& Functor)
    {
        ElementType* RESTRICT CurrentAddress = GetData();
        ElementType* RESTRICT EndAddress     = GetData() + ArraySize;
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
        TArray TempArray(Move(*this));
        MoveFrom(Move(Other));
        Other.MoveFrom(Move(TempArray));
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
        return (Address >= GetData()) && (Address < (GetData() + ArrayCapacity));
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
        return *GetData();
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        CHECK(!IsEmpty());
        return *GetData();
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE ElementType& LastElement() noexcept
    {
        CHECK(!IsEmpty());
        return *(GetData() + (ArraySize - 1));
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const ElementType& LastElement() const noexcept
    {
        CHECK(!IsEmpty());
        return *(GetData() + (ArraySize - 1));
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE ElementType* GetData() noexcept
    {
        return Allocator.GetAllocation();
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const ElementType* GetData() const noexcept
    {
        return Allocator.GetAllocation();
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
    NODISCARD FORCEINLINE SizeType GetSize() const noexcept
    {
        return ArraySize;
    }

    /**
     * @return - Returns the stride of each element of the container
     */
    NODISCARD CONSTEXPR SizeType GetStride() const noexcept
    {
        return sizeof(ElementType);
    }

    /**
     * @return - Returns the size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return GetSize() * sizeof(ElementType);
    }

    /**
     * @return - Returns the capacity of the container
     */
    NODISCARD FORCEINLINE SizeType GetCapacity() const noexcept
    {
        return ArrayCapacity;
    }

    /**
     * @return - Returns the capacity of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return GetCapacity() * sizeof(ElementType);
    }

    /**
     * @brief       - Retrieve a element at a certain index of the array
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& GetElementAt(SizeType Index) noexcept
    {
        CHECK(Index < ArraySize);
        return *(GetData() + Index);
    }

    /**
     * @brief       - Retrieve a element at a certain index of the array
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& GetElementAt(SizeType Index) const noexcept
    {
        CHECK(Index < ArraySize);
        return *(GetData() + Index);
    }

    /**
     * @brief  - Create an array-view of the array
     * @return - A new array-view pointing this array's data
     */
    NODISCARD FORCEINLINE TArrayView<ElementType> CreateView() noexcept
    {
        return TArrayView<ElementType>(GetData(), GetSize());
    }

    /**
     * @brief  - Create an array-view of the array
     * @return - A new array-view pointing this array's data
     */
    NODISCARD FORCEINLINE TArrayView<const ElementType> CreateView() const noexcept
    {
        return TArrayView<const ElementType>(GetData(), GetSize());
    }

    /**
     * @brief             - Create an array-view of the array
     * @param Offset      - Offset into the array
     * @param NumElements - Number of elements to include in the view
     * @return            - A new array-view pointing to the specified elements
     */
    NODISCARD FORCEINLINE TArrayView<ElementType> CreateView(SizeType Offset, SizeType NumElements) noexcept
    {
        CHECK((NumElements < ArraySize) && (Offset + NumElements < ArraySize));
        return TArrayView<ElementType>(GetData() + Offset, NumElements);
    }

    /**
     * @brief             - Create an array-view of the array
     * @param Offset      - Offset into the array
     * @param NumElements - Number of elements to include in the view
     * @return            - A new array-view pointing to the specified elements
     */
    NODISCARD FORCEINLINE TArrayView<const ElementType> CreateView(SizeType Offset, SizeType NumElements) const noexcept
    {
        CHECK((NumElements < ArraySize) && (Offset + NumElements < ArraySize));
        return TArrayView<const ElementType>(GetData() + Offset, NumElements);
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
        return *GetData();
    }

    /**
     * @brief  - Retrieve the top of the heap. The same as the first element.
     * @return - A reference to the element at the top of the heap
     */
    NODISCARD FORCEINLINE const ElementType& HeapTop() const noexcept
    {
        return *GetData();
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
        Insert(0, Forward<ElementType>(Element));
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
    FORCEINLINE void HeapSort()
    {
        Heapify();
        for (SizeType Index = ArraySize - 1; Index > 0; --Index)
        {
            ::Swap<ElementType>(GetElementAt(0), GetElementAt(Index));
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
        MoveFrom(Forward<TArray>(Other));
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
        return (GetSize() == Other.GetSize()) ? ::CompareElements<ElementType>(GetData(), Other.GetData(), GetSize()) : (false);
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
        return GetElementAt(Index);
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept
    {
        return GetElementAt(Index);
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
        return IteratorType(*this, GetSize());
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
        return ConstIteratorType(*this, GetSize());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, GetSize());
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
        return ReverseConstIteratorType(*this, GetSize());
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
        if (ArrayCapacity < NumElements)
        {
            Allocator.Realloc(ArrayCapacity, NumElements);
            ArrayCapacity = NumElements;
        }

        ArraySize = NumElements;
    }

    FORCEINLINE void ConstructEmpty(SizeType NumElements)
    {
        CreateUnitialized(NumElements);
        ::DefaultConstructElements<ElementType>(GetData(), NumElements);
    }

    FORCEINLINE void ConstructFrom(SizeType NumElements, const ElementType& Element)
    {
        CreateUnitialized(NumElements);
        ::ConstructElementsFrom<ElementType>(GetData(), NumElements, Element);
    }

    FORCEINLINE void CopyConstructFrom(const ElementType* Elements, SizeType NumElements, SizeType ExtraCapacity)
    {
        const SizeType NewSize = NumElements + ExtraCapacity;
        CreateUnitialized(NewSize);
        ::CopyConstructElements<ElementType>(GetData(), Elements, NumElements);
    }

    FORCEINLINE void MoveFrom(TArray&& FromArray)
    {
        if (FromArray.GetData() != GetData())
        {
            // Since the memory remains the same we should not need to use move-assignment or constructor. However, still need to call destructors
            ::DestroyElements<ElementType>(GetData(), GetSize());
            Allocator.MoveFrom(Move(FromArray.Allocator));

            ArraySize               = FromArray.ArraySize;
            ArrayCapacity           = FromArray.ArrayCapacity;
            FromArray.ArraySize     = 0;
            FromArray.ArrayCapacity = 0;
        }
    }

    FORCEINLINE void ReserveUnchecked(const SizeType NewCapacity) noexcept
    {
        if CONSTEXPR (TNot<TIsReallocatable<ElementType>>::Value)
        {
            if (ArrayCapacity)
            {
                // For non-trivial objects a new allocator is necessary in order to correctly reallocate objects. This in case
                // objects has references to themselves or "child-objects" that references these objects.
                AllocatorType NewAllocator;
                NewAllocator.Realloc(ArrayCapacity, NewCapacity);
                if (ArraySize)
                {
                    ::RelocateElements<ElementType>(NewAllocator.GetAllocation(), Allocator.GetAllocation(), ArraySize);
                }

                Allocator.MoveFrom(Move(NewAllocator));
            }
            else
            {
                Allocator.Realloc(ArrayCapacity, NewCapacity);
            }
        }
        else
        {
            Allocator.Realloc(ArrayCapacity, NewCapacity);
        }

        ArrayCapacity = NewCapacity;
    }

    FORCEINLINE void InsertUninitializedUnchecked(const SizeType Position, const SizeType NumElements) noexcept
    {
        ExpandStorage(NumElements);
        ElementType* const CurrentAddress = GetData() + Position;
        ::RelocateElements<ElementType>(CurrentAddress + NumElements, CurrentAddress, ArraySize - Position);
    }

    FORCEINLINE void PopRangeUnchecked(SizeType NumElements) noexcept
    {
        const SizeType NewArraySize = ArraySize - NumElements;
        ::DestroyElements<ElementType>(GetData() + NewArraySize, NumElements);
        ArraySize = NewArraySize;
    }

    FORCEINLINE void ExpandStorage() noexcept
    {
        ExpandStorage(1);
    }

    FORCEINLINE void ExpandStorage(SizeType NumElements) noexcept
    {
        if (ArraySize + NumElements > ArrayCapacity)
        {
            const SizeType NewCapacity = CalculateExpandCapacity(NumElements, ArrayCapacity);
            ReserveUnchecked(NewCapacity);
        }
    }

    // TODO: Better to have top in back? Better to do recursive?
    FORCEINLINE void Heapify(SizeType Size, SizeType Index) noexcept
    {
        SizeType StartIndex = Index;
        SizeType Largest    = Index;

        while (true)
        {
            const SizeType Left  = LeftIndex(StartIndex);
            const SizeType Right = RightIndex(StartIndex);

            if (Left < Size && GetElementAt(Left) > GetElementAt(Largest))
            {
                Largest = Left;
            }

            if (Right < Size && GetElementAt(Right) > GetElementAt(Largest))
            {
                Largest = Right;
            }

            if (Largest != StartIndex)
            {
                ::Swap<ElementType>(GetElementAt(StartIndex), GetElementAt(Largest));
                StartIndex = Largest;
            }
            else
            {
                break;
            }
        }
    }

private:
    NODISCARD
    static FORCEINLINE SizeType LeftIndex(SizeType Index)
    {
        return (2 * Index + 1);
    }

    NODISCARD
    static FORCEINLINE SizeType RightIndex(SizeType Index)
    {
        return (2 * Index + 2);
    }

    // Calculate how much the array should grow, will always be at least one
    NODISCARD
    static FORCEINLINE SizeType CalculateExpandCapacity(SizeType NumElements, SizeType CurrentCapacity) noexcept
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
    SizeType      ArrayCapacity;
};


template<
    typename T,
    typename AllocatorType>
struct TIsTArrayType<TArray<T, AllocatorType>>
{
    enum { Value = true };
};

template<
    typename T,
    typename AllocatorType>
struct TIsContiguousContainer<TArray<T, AllocatorType>>
{
    enum { Value = true };
};


template<typename T, typename AllocatorType>
inline TUniquePtr<T[]> MakeUniquePtr(const TArray<T, AllocatorType>& Array) noexcept
{
    T* Memory = FMemory::Malloc<T>(Array.GetSize());
    ::CopyConstructElements<T>(Memory, Array.GetData(), Array.GetSize());
    return TUniquePtr<T[]>(Memory);
}
