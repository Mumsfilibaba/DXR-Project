#pragma once
#include "Iterator.h"
#include "Allocators.h"
#include "UniquePtr.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsPointer.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/IsArray.h"

#include <initializer_list>

/* Dynamic Array similar to std::vector */
template<typename T, typename AllocatorType = TDefaultAllocator<T>>
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
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
    }

    /* Default creates a certain number of elements */
    FORCEINLINE explicit TArray( SizeType InSize ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        EmptyConstruct( InSize );
    }

    /* Allocates the specified amount of elements, and initializes them to the same value */
    FORCEINLINE explicit TArray( SizeType InSize, const ElementType& Element ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        EmptyConstructFrom( InSize, Element );
    }

    /* Creates an array from a raw pointer array */
    FORCEINLINE explicit TArray( const ElementType* InputArray, SizeType Count ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        EmptyCopyFrom( InputArray, Count );
    }

    /* Creates an array from an std::initializer_list */
    FORCEINLINE TArray( std::initializer_list<ElementType> InitList ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        EmptyCopyFrom( InitList.begin(), static_cast<SizeType>(InitList.size()) );
    }

    /* Copy-constructs an array from another array */
    FORCEINLINE TArray( const TArray& Other ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        EmptyCopyFrom( Other.Data(), Other.Size() );
    }

    /* Copy-constructs an array from another array */
    template<typename ArrayType>
    FORCEINLINE TArray( const ArrayType& Other ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        EmptyCopyFrom( Other.Data(), Other.Size() );
    }

    /* Move-constructs an array from another array */
    FORCEINLINE TArray( TArray&& Other ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        MoveFrom( Forward<TArray>( Other ) );
    }

    FORCEINLINE ~TArray()
    {
        Empty();
    }

    /* Empties the array and deallocates the memory */
    FORCEINLINE void Empty() noexcept
    {
        if ( ArraySize )
        {
            DestructRange<ElementType>( Data(), ArraySize );
            ArraySize = 0;
        }

        Allocator.Free();
        ArrayCapacity = 0;
    }

    /* Clears the container, but does not deallocate the memory */
    FORCEINLINE void Clear() noexcept
    {
        DestructRange<ElementType>( Data(), ArraySize );
        ArraySize = 0;
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( SizeType NewSize = 0 ) noexcept
    {
        DestructRange<ElementType>( Data(), ArraySize );

        if ( NewSize )
        {
            Construct( NewSize );
        }
        else
        {
            ArraySize = 0;
        }
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( SizeType NewSize, const ElementType& Element ) noexcept
    {
        DestructRange<ElementType>( Data(), ArraySize );

        if ( NewSize )
        {
            EmptyConstructFrom( NewSize, Element );
        }
        else
        {
            ArraySize = 0;
        }
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( const ElementType* InputArray, SizeType Count ) noexcept
    {
        Assert( (InputArray != nullptr) && Count );

        DestructRange<ElementType>( Data(), ArraySize );

        if ( Count )
        {
            EmptyCopyFrom( InputArray, Count );
        }
        else
        {
            ArraySize = 0;
        }
    }

    /* Resets the container, but does not deallocate the memory */
    template<typename ArrayType>
    FORCEINLINE void Reset( const ArrayType& InputArray ) noexcept
    {
        if ( this != &InputArray )
        {
            Reset( InputArray.Data(), InputArray.Size() );
        }
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( TArray&& InputArray ) noexcept
    {
        if ( this != &InputArray )
        {
            DestructRange<ElementType>( Data(), ArraySize );
            MoveFrom( InputArray );
        }
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( std::initializer_list<ElementType> InitList ) noexcept
    {
        Reset( InitList.begin(), static_cast<SizeType>(InitList.size()) );
    }

    /* Fills the container with the specified value */
    FORCEINLINE void Fill( const ElementType& InputElement ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = InputElement;
        }
    }

    /* Resizes the container with a new size, and default constructs them*/
    inline void Resize( SizeType NewSize ) noexcept
    {
        if ( NewSize > ArraySize )
        {
            if ( NewSize >= ArrayCapacity )
            {
                ReserveStorage( NewSize );
            }

            DefaultConstructRange<ElementType>( Data() + ArraySize, ArraySize - NewSize );
            ArraySize = NewSize;
        }
        else if ( NewSize < ArraySize )
        {
            RemoveFromEnd( ArraySize - NewSize );
        }
    }

    /* Resizes the container with a new size, and constructs them with value */
    inline void Resize( SizeType NewSize, const ElementType& Element ) noexcept
    {
        if ( NewSize > ArraySize )
        {
            if ( NewSize >= ArrayCapacity )
            {
                ReserveStorage( NewSize );
            }

            ConstructRangeFrom<ElementType>( Data() + ArraySize, NewSize - ArraySize, Element );
            ArraySize = NewSize;
        }
        else if ( NewSize < ArraySize )
        {
            RemoveFromEnd( ArraySize - NewSize );
        }
    }

    /* Resizes the allocation */
    inline void Reserve( SizeType NewCapacity ) noexcept
    {
        if ( NewCapacity != ArrayCapacity )
        {
            if ( NewCapacity < ArraySize )
            {
                DestructRange<ElementType>( Data() + NewCapacity, ArraySize - NewCapacity );
                ArraySize = NewCapacity;
            }

            ReserveStorage( NewCapacity );
        }
    }

    /* Constructs a new element at the end */
    template<typename... ArgTypes>
    inline ElementType& EmplaceBack( ArgTypes&&... Args ) noexcept
    {
        if ( ArraySize == ArrayCapacity )
        {
            const SizeType NewCapacity = GetGrowCapacity( ArraySize + 1, ArrayCapacity );
            ReserveStorage( NewCapacity );
        }

        new(Data() + (ArraySize++)) ElementType( Forward<ArgTypes>( Args )... );
        return LastElement();
    }

    /* Inserts a new element at the end */
    FORCEINLINE ElementType& PushBack( const ElementType& Element ) noexcept
    {
        return EmplaceBack( Element );
    }

    /* Inserts a new element at the end */
    FORCEINLINE ElementType& PushBack( ElementType&& Element ) noexcept
    {
        return EmplaceBack( Forward<ElementType>( Element ) );
    }

    /* Constructs a new element at specified position */
    template<typename... ArgTypes>
    inline void EmplaceAt( SizeType Position, ArgTypes&&... Args ) noexcept
    {
        Assert( Position <= ArraySize );

        /* Simple path if position is last */
        if ( Position == ArraySize )
        {
            EmplaceBack( Forward<ArgTypes>( Args )... );
        }
        else
        {
            /* Make room within array and allocate if necessary */
            ReserveForInsertion( Position, 1 );
            new(Data() + Position) ElementType( Forward<ArgTypes>( Args )... );

            /* Update size */
            ArraySize++;
        }
    }

    /* Inserts an element at the specified position */
    FORCEINLINE void InsertAt( SizeType Position, const ElementType& Element ) noexcept
    {
        EmplaceAt( Position, Element );
    }

    /* Inserts an element at the specified position */
    FORCEINLINE void InsertAt( SizeType Position, ElementType&& Element ) noexcept
    {
        EmplaceAt( Position, Forward<ElementType>( Element ) );
    }

    /* Insert an array into the container at the position */
    inline void InsertAt( SizeType Position, const ElementType* InputArray, SizeType Count ) noexcept
    {
        Assert( Position <= ArraySize );

        /* Special case if at the end */
        if ( Position == ArraySize )
        {
            Append( InputArray, Count );
        }
        else
        {
            Assert( InputArray != nullptr );

            /* Make room within array and allocate if necessary */
            ReserveForInsertion( Position, Count );
            CopyConstructRange<ElementType>( Data() + Position, InputArray, Count );

            /* Update size */
            ArraySize += Count;
        }
    }

    /* Insert an std::initializer_list into the container at the position */
    FORCEINLINE void InsertAt( SizeType Position, std::initializer_list<ElementType> InitList ) noexcept
    {
        InsertAt( Position, InitList.begin(), static_cast<SizeType>(InitList.size()) );
    }

    /* Insert an array into the container at the position */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TNot<TIsArray<ArrayType>>::Value>::Type InsertAt( SizeType Position, const ArrayType& InArray ) noexcept
    {
        InsertAt( Position, InArray.Data(), InArray.Size() );
    }

    /* Inserts an array at the end */
    inline void Append( const ElementType* InputArray, SizeType Count ) noexcept
    {
        Assert( InputArray != nullptr );

        const SizeType NewSize = ArraySize + Count;
        if ( NewSize >= ArrayCapacity )
        {
            const SizeType NewCapacity = GetGrowCapacity( ArraySize + Count, ArrayCapacity );
            ReserveStorage( NewCapacity );
        }

        CopyConstructRange<ElementType>( Data() + ArraySize, InputArray, Count );
        ArraySize = NewSize;
    }

    /* Inserts an array at the end */
    template<typename ArrayType>
    FORCEINLINE void Append( const ArrayType& Other ) noexcept
    {
        Append( Other.Data(), Other.Size() );
    }

    /* Inserts an array at the end */
    FORCEINLINE void Append( std::initializer_list<ElementType> InitList ) noexcept
    {
        Append( InitList.begin(), static_cast<SizeType>(InitList.size()) );
    }

    /* Removes a number of elments from the back */
    FORCEINLINE void PopBackRange( SizeType Count ) noexcept
    {
        if ( !IsEmpty() )
        {
            RemoveFromEnd( Count );
        }
    }

    /* Removes the last element */
    FORCEINLINE void PopBack() noexcept
    {
        PopBackRange( 1 );
    }

    /* Remove a range starting at position and containg count number of elements */
    inline void RemoveRangeAt( SizeType Position, SizeType Count ) noexcept
    {
        Assert( Position + Count <= ArraySize );

        if ( Position + Count == ArraySize )
        {
            RemoveFromEnd( Count );
        }
        else
        {
            DestructRange<ElementType>( Data() + Position, Count );
            RelocateRange<ElementType>( Data() + Position, Data() + Position + Count, ArraySize - (Position + Count) );
            ArraySize = ArraySize - Count;
        }
    }

    /* Removes the element at the position */
    FORCEINLINE void RemoveAt( SizeType Position ) noexcept
    {
        RemoveRangeAt( Position, 1 );
    }

    /* Removes the element pointed to by a iterator at the position */
    FORCEINLINE IteratorType Remove( IteratorType Iterator ) noexcept
    {
        Assert( Iterator.IsFrom( *this ) );
        RemoveAt( Iterator.GetIndex() );
        return Iterator;
    }

    /* Removes the element pointed to by a iterator at the position */
    FORCEINLINE ConstIteratorType Remove( ConstIteratorType Iterator ) noexcept
    {
        Assert( Iterator.IsFrom( *this ) );
        RemoveAt( Iterator.GetIndex() );
        return Iterator;
    }

    /* Swaps container with another */
    FORCEINLINE void Swap( TArray& Other ) noexcept
    {
        TArray Temp( Move( *this ) );
        *this = Move( Other );
        Other = Move( Temp );
    }

    /* Shrinks the allocation to be the same as the size */
    FORCEINLINE void ShrinkToFit() noexcept
    {
        Reserve( ArraySize );
    }

    /* Checks if there are any elements */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ArraySize == 0) && (Data() != nullptr);
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType( *this, 0 );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType( *this, Size() );
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType( *this, 0 );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType( *this, 0 );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, 0 );
    }

    /* Returns the first element of the container */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert( !IsEmpty() );
        return Data()[0];
    }

    /* Returns the first element of the container */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert( !IsEmpty() );
        return Data()[0];
    }

    /* Returns the last element of the container */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert( !IsEmpty() );
        return Data()[ArraySize - 1];
    }

    /* Returns the last element of the container */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert( !IsEmpty() );
        return Data()[ArraySize - 1];
    }

    /* Returns the data of the container */
    FORCEINLINE ElementType* Data() noexcept
    {
        return Allocator.Raw();
    }

    /* Returns the data of the container */
    FORCEINLINE const ElementType* Data() const noexcept
    {
        return Allocator.Raw();
    }

    /* Returns the last valid index the container */
    FORCEINLINE SizeType LastIndex() const noexcept
    {
        return ArraySize > 0 ? ArraySize - 1 : 0;
    }

    /* Returns the size of the container */
    FORCEINLINE SizeType Size() const noexcept
    {
        return ArraySize;
    }

    /* Returns the size of the container in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof( ElementType );
    }

    /* Returns the capacity of the container */
    FORCEINLINE SizeType Capacity() const noexcept
    {
        return ArrayCapacity;
    }

    /* Returns the capacity of the container in bytes */
    FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return Capacity() * sizeof( ElementType );
    }

    /* Returns the element at a certain index */
    FORCEINLINE ElementType& At( SizeType Index ) noexcept
    {
        Assert( Index < ArraySize );
        return Data()[Index];
    }

    /* Returns the element at a certain index */
    FORCEINLINE const ElementType& At( SizeType Index ) const noexcept
    {
        Assert( Index < ArraySize );
        return Data()[Index];
    }

    /* Allocate and copy the contents into a uniqueptr */
    FORCEINLINE TUniquePtr<ElementType[]> GetUniquePtr() const noexcept
    {
        ElementType* Memory = Memory::Malloc<ElementType>( Size() );
        CopyConstructRange<ElementType>( Memory, Data(), Size() );
        return TUniquePtr<ElementType[]>( Memory );
    }

    /* Create a min-heap of the array */
    inline void MinHeapify() noexcept
    {
        const SizeType StartIndex = (ArraySize / 2) - 1;
        for ( SizeType i = StartIndex; i >= 0; i-- )
        {
            MinHeapify( ArraySize, i );
        }
    }

    /* Create a max-heap of the array */
    inline void MaxHeapify() noexcept
    {
        const SizeType StartIndex = (ArraySize / 2) - 1;
        for ( SizeType i = StartIndex; i >= 0; i-- )
        {
            MaxHeapify( ArraySize, i );
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

    /* Inserts a new element at the top of the min-heap */
    FORCEINLINE void MinHeapPush( const ElementType& Element ) noexcept
    {
        InsertAt( 0, Element );
        MinHeapify( ArraySize, 0 );
    }

    /* Inserts a new element at the top of the min-heap */
    FORCEINLINE void MinHeapPush( ElementType&& Element ) noexcept
    {
        InsertAt( 0, Forward<ElementType>( Element ) );
        MinHeapify( ArraySize, 0 );
    }

    /* Removes the top of the min-heap */
    FORCEINLINE void MinHeapPop( ElementType& OutElement ) noexcept
    {
        OutElement = HeapTop();
        MinHeapPop();
    }

    /* Removes the top of the min-heap */
    FORCEINLINE void MinHeapPop() noexcept
    {
        RemoveAt( 0 );
        MinHeapify();
    }

    /* Performs heap sort on the array ( assuming < operator )*/
    FORCEINLINE void MinHeapSort()
    {
        MinHeapify();

        for ( SizeType Index = ArraySize - 1; Index > 0; Index-- )
        {
            SwapElements( 0, Index );
            MinHeapify( Index, 0 );
        }
    }

    /* Inserts a new element at the top of the max-heap */
    FORCEINLINE void MaxHeapPush( const ElementType& Element ) noexcept
    {
        InsertAt( 0, Element );
        MaxHeapify( ArraySize, 0 );
    }

    /* Inserts a new element at the top of the max-heap */
    FORCEINLINE void MaxHeapPush( ElementType&& Element ) noexcept
    {
        InsertAt( 0, Forward<ElementType>( Element ) );
        MaxHeapify( ArraySize, 0 );
    }

    /* Removes the top of the max-heap */
    FORCEINLINE void MaxHeapPop( ElementType& OutElement ) noexcept
    {
        OutElement = HeapTop();
        MaxHeapPop();
    }

    /* Removes the top of the max-heap */
    FORCEINLINE void MaxHeapPop() noexcept
    {
        RemoveAt( 0 );
        MaxHeapify();
    }

    /* Performs heap sort on the array ( assuming > operator )*/
    FORCEINLINE void MaxHeapSort()
    {
        MaxHeapify();

        for ( SizeType Index = ArraySize - 1; Index > 0; Index-- )
        {
            SwapElements( 0, Index );
            MaxHeapify( Index, 0 );
        }
    }

    /* Sets the container to another array by copying it */
    FORCEINLINE TArray& operator=( const TArray& Other ) noexcept
    {
        Reset( Other );
        return *this;
    }

    /* Sets the container to another array by moving it */
    FORCEINLINE TArray& operator=( TArray&& Other ) noexcept
    {
        MoveFrom( Forward<TArray>( Other ) );
        return *this;
    }

    /* Sets the container to a std::initializer_list */
    FORCEINLINE TArray& operator=( std::initializer_list<ElementType> InitList ) noexcept
    {
        Reset( InitList );
        return *this;
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename ArrayType>
    FORCEINLINE bool operator==( const ArrayType& RHS ) const noexcept
    {
        if ( Size() != RHS.Size() )
        {
            return false;
        }

        return CompareRange<ElementType>( Data(), RHS.Data(), Size() );
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename ArrayType>
    FORCEINLINE bool operator!=( const ArrayType& RHS ) const noexcept
    {
        return !(*this == RHS);
    }

    /* Returns the elment at a certain index */
    FORCEINLINE ElementType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    /* Returns the elment at a certain index */
    FORCEINLINE const ElementType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
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

    FORCEINLINE void InitUnitialized( SizeType Count )
    {
        if ( ArrayCapacity < Count )
        {
            Allocator.Allocate( Count );
            ArrayCapacity = Count;
        }

        ArraySize = Count;
    }

    /* Default-construct array, assumes that the container is currently empty */
    FORCEINLINE void EmptyConstruct( SizeType Count )
    {
        InitUnitialized( Count );
        DefaultConstructRange<ElementType>( Data(), Count );
    }

    /* Construct array from Element, assumes that the container is currently empty */
    FORCEINLINE void EmptyConstructFrom( SizeType Count, const ElementType& Element )
    {
        InitUnitialized( Count );
        ConstructRangeFrom<ElementType>( Data(), Count, Element );
    }

    /* Copy from array, assumes that the container is currently empty */
    FORCEINLINE void EmptyCopyFrom( const ElementType* From, SizeType Count )
    {
        InitUnitialized( Count );
        CopyConstructRange<ElementType>( Data(), From, Count );
    }

    FORCEINLINE void MoveFrom( TArray&& FromArray )
    {
        /* Since the memory remains the same we should not need to use move-assignment or constructor.
           However, still need to call our destructors */
        DestructRange<ElementType>( Data(), Size() );
        Allocator.MoveFrom( Move( FromArray.Allocator ) );

        ArraySize = FromArray.ArraySize;
        ArrayCapacity = FromArray.ArrayCapacity;
        FromArray.ArraySize = 0;
        FromArray.ArrayCapacity = 0;
    }

    FORCEINLINE void ReserveStorage( const SizeType NewCapacity ) noexcept
    {
        /* Simple Memory::Realloc for trivial elements */
        if constexpr ( !TIsTrivial<ElementType>::Value )
        {
            if ( ArrayCapacity )
            {
                /* For non-trivial objects a new allocator is necessary in order to correctly relocate objects. This in case
                    objects has references to themselves or childobjects that references these objects. */
                AllocatorType NewAllocator;
                NewAllocator.Allocate( NewCapacity );

                /* Relocate existing elements */
                RelocateRange<ElementType>( NewAllocator.Raw(), Data(), ArraySize );

                /* Move allocator */
                Allocator.MoveFrom( Move( NewAllocator ) );
            }
            else
            {
                /* Allocate new memory */
                Allocator.Allocate( NewCapacity );
            }
        }
        else
        {
            Allocator.Allocate( NewCapacity );
        }

        ArrayCapacity = NewCapacity;
    }

    /* Reserves room and not necessarily new memory */
    FORCEINLINE void ReserveForInsertion( const SizeType InsertAt, const SizeType ElementCount ) noexcept
    {
        const SizeType NewSize = ArraySize + ElementCount;
        if ( NewSize >= ArrayCapacity )
        {
            const SizeType NewCapacity = GetGrowCapacity( NewSize, ArrayCapacity );
            Assert( NewCapacity >= ArrayCapacity );

            /* Simpler path for trivial elements */
            if constexpr ( !TIsTrivial<ElementType>::Value )
            {
                /* For non-trivial objects a new allocator is necessary in order to correctly relocate objects. This in case
                   objects has references to themselves or childobjects that references these objects. */
                AllocatorType NewAllocator;
                NewAllocator.Allocate( NewCapacity );

                /* Elements before new area */
                RelocateRange<ElementType>( NewAllocator.Raw(), Data(), InsertAt );

                /* Elements after new area */
                RelocateRange<ElementType>( NewAllocator.Raw() + InsertAt + ElementCount, Data() + InsertAt, ArraySize - InsertAt );
                Allocator.MoveFrom( Move( NewAllocator ) );
            }
            else
            {
                Allocator.Allocate( NewCapacity );

                /* Elements after new area */
                RelocateRange<ElementType>( Data() + InsertAt + ElementCount, Data() + InsertAt, ArraySize - InsertAt );
            }

            /* Update capacity */
            ArrayCapacity = NewCapacity;
        }
        else
        {
            /* Elements after new area */
            RelocateRange<ElementType>( Data() + InsertAt + ElementCount, Data() + InsertAt, ArraySize - InsertAt );
        }
    }

    FORCEINLINE void RemoveFromEnd( SizeType Count ) noexcept
    {
        ArraySize = ArraySize - Count;
        DestructRange<ElementType>( Data() + ArraySize, Count );
    }

    // Get index of left child of node at index
    static FORCEINLINE SizeType LeftIndex( SizeType Index )
    {
        return (2 * Index + 1);
    }

    // Get index of right child of node at index
    static FORCEINLINE SizeType RightIndex( SizeType Index )
    {
        return (2 * Index + 2);
    }

    FORCEINLINE void SwapElements( SizeType FirstIndex, SizeType SecondIndex )
    {
        ElementType Temp( Move( At( FirstIndex ) ) );
        At( FirstIndex ) = Move( At( SecondIndex ) );
        At( SecondIndex ) = Move( Temp );
    }

    // TODO: Better to have top in back? Better to do recursive?

    /* Internal create a min-heap of the array */
    FORCEINLINE void MinHeapify( SizeType Size, SizeType Index ) noexcept
    {
        SizeType StartIndex = Index;
        SizeType Smallest = Index;

        while ( true )
        {
            const SizeType Left = LeftIndex( StartIndex );
            const SizeType Right = RightIndex( StartIndex );

            if ( Left < Size && At( Left ) < At( Smallest ) )
            {
                Smallest = Left;
            }

            if ( Right < Size && At( Right ) < At( Smallest ) )
            {
                Smallest = Right;
            }

            if ( Smallest != StartIndex )
            {
                SwapElements( StartIndex, Smallest );
                StartIndex = Smallest;
            }
            else
            {
                break;
            }
        }
    }

    /* Internal create a max-heap of the array */
    FORCEINLINE void MaxHeapify( SizeType Size, SizeType Index ) noexcept
    {
        SizeType StartIndex = Index;
        SizeType Largest = Index;

        while ( true )
        {
            const SizeType Left = LeftIndex( StartIndex );
            const SizeType Right = RightIndex( StartIndex );

            if ( Left < Size && At( Left ) > At( Largest ) )
            {
                Largest = Left;
            }

            if ( Right < Size && At( Right ) > At( Largest ) )
            {
                Largest = Right;
            }

            if ( Largest != StartIndex )
            {
                SwapElements( StartIndex, Largest );
                StartIndex = Largest;
            }
            else
            {
                break;
            }
        }
    }

    static FORCEINLINE SizeType GetGrowCapacity( SizeType NewSize, SizeType CurrentCapacity ) noexcept
    {
        return NewSize + (CurrentCapacity);
    }

    /* Allocator contains the pointer */
    AllocatorType Allocator;
    SizeType      ArraySize;
    SizeType      ArrayCapacity;
};
