#pragma once
#include "Iterator.h"
#include "Allocators.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsPointer.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/ObjectHandling.h"

#include <initializer_list>

/* Dynamic Array similar to std::vector */
template<typename T, typename AllocatorType = TDefaultAllocator<T>>
class TArray
{
public:
    typedef T                                   ElementType;
    typedef ElementType* Iterator;
    typedef const ElementType* ConstIterator;
    typedef TReverseIterator<ElementType>       ReverseIterator;
    typedef TReverseIterator<const ElementType> ConstReverseIterator;
    typedef uint32                              SizeType;

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
        InternalConstruct( InSize );
    }

    /* Allocates the specified amount of elements, and initializes them to the same value */
    FORCEINLINE explicit TArray( SizeType InSize, const ElementType& Element ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        InternalConstructFrom( InSize, Element );
    }

    /* Creates an array from a raw pointer array */
    FORCEINLINE TArray( const ElementType* InputArray, SizeType Count ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        InternalCopyFrom( InputArray, Count );
    }

    /* Creates an array from an std::initializer_list */
    FORCEINLINE TArray( std::initializer_list<ElementType> InitList ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        InternalCopyFrom( InitList.begin(), InitList.size() );
    }

    /* Copy-constructs an array from another array */
    FORCEINLINE TArray( const TArray& Other ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        InternalCopyFrom( Other.Data(), Other.Size() );
    }

    /* Move-constructs an array from another array */
    FORCEINLINE TArray( TArray&& Other ) noexcept
        : Allocator()
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
    {
        InternalMoveFrom( ::Forward<TArray>( Other ) );
    }

    FORCEINLINE ~TArray()
    {
        Reset();
        ArrayCapacity = 0;
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( SizeType NewSize = 0 ) noexcept
    {
        DestructRange<ElementType>( Data(), ArraySize );
        InternalConstruct( NewSize );

        ArraySize = NewSize;
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( SizeType NewSize, const ElementType& Element ) noexcept
    {
        DestructRange<ElementType>( Data(), ArraySize );
        InternalConstructFrom( NewSize, Element );

        ArraySize = NewSize;
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( const ElementType* InputArray, SizeType Count ) noexcept
    {
        DestructRange<ElementType>( Data(), ArraySize );
        InternalCopyFrom( InputArray, Count );

        ArraySize = Count;
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( const TArray& InputArray ) noexcept
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
            InternalMoveFrom( InputArray );
        }
    }

    /* Resets the container, but does not deallocate the memory */
    FORCEINLINE void Reset( std::initializer_list<ElementType> InitList ) noexcept
    {
        Reset( InitList.begin(), InitList.size() );
    }

    /* Fills the container with the specified value */
    FORCEINLINE void Fill( const ElementType& Element ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = Element;
        }
    }

    /* Fills the container with the specified value */
    FORCEINLINE void Fill( ElementType&& Element ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = ::Move( Element );
        }
    }

    /* Resizes the container with a new size, and default constructs them*/
    FORCEINLINE void Resize( SizeType NewSize ) noexcept
    {
        if ( NewSize > ArraySize )
        {
            InternalReserve( NewSize );
            DefaultConstructRange<ElementType>( Data() + ArraySize, Data() + NewSize );
            ArraySize = NewSize;
        }
        else if ( NewSize < ArraySize )
        {
            PopBackNum( ArraySize - NewSize );
        }
    }

    /* Resizes the container with a new size, and constructs them with value */
    FORCEINLINE void Resize( SizeType NewSize, const ElementType& Element ) noexcept
    {
        if ( NewSize > ArraySize )
        {
            InternalReserve( NewSize );
            ConstructRangeFrom<ElementType>( Data() + ArraySize, Data() + NewSize, Element );
            ArraySize = NewSize;
        }
        else if ( NewSize < ArraySize )
        {
            PopBackNum( ArraySize - NewSize );
        }
    }

    /* Resizes the allocation */
    FORCEINLINE void Reserve( SizeType NewCapacity ) noexcept
    {
        if ( NewCapacity != ArrayCapacity )
        {
            if ( NewCapacity < ArraySize )
            {
                DestructRange<ElementType>( Data() + NewCapacity, ArraySize - NewCapacity );
            }

            InternalReserve( NewCapacity );
        }
    }

    /* Constructs a new element at the end */
    template<typename... ArgTypes>
    FORCEINLINE ElementType& EmplaceBack( ArgTypes&&... Args ) noexcept
    {
        if ( ArraySize == ArrayCapacity )
        {
            const SizeType NewCapacity = InternalGrowCapacity( ArraySize + 1, ArrayCapacity );
            InternalReserve( NewCapacity );
        }

        ElementType* DataEnd = Data() + (ArraySize++);
        new(reinterpret_cast<void*>(DataEnd)) ElementType( ::Forward<ArgTypes>( Args )... );
        return (*DataEnd);
    }

    /* Inserts a new element at the end */
    FORCEINLINE ElementType& PushBack( const ElementType& Element ) noexcept
    {
        return EmplaceBack( Element );
    }

    /* Inserts a new element at the end */
    FORCEINLINE ElementType& PushBack( ElementType&& Element ) noexcept
    {
        return EmplaceBack( ::Forward<ElementType>( Element ) );
    }

    /* Constructs a new element at specified position */
    template<typename... ArgTypes>
    inline void EmplaceAt( SizeType Position, ArgTypes&&... Args ) noexcept
    {
        Assert( Position <= ArraySize );

        /* Simple path if position is last */
        if ( Position == ArraySize )
        {
            EmplaceBack( ::Forward<ArgTypes>( Args )... );
            return;
        }

        if ( ArraySize >= ArrayCapacity )
        {
            const SizeType NewCapacity = InternalGrowCapacity( ArraySize + 1, ArrayCapacity );
            InternalReserveForInsertion( NewCapacity, Position, 1 );
        }

        ElementType* DataEnd = Data() + (ArraySize++);
        new(reinterpret_cast<void*>(DataEnd)) ElementType( ::Forward<ArgTypes>( Args )... );
        return Position;
    }

    /* Inserts the Value at the specified position */
    FORCEINLINE void InsertAt( SizeType Position, const ElementType& Element ) noexcept
    {
        EmplaceAt( Position, Element );
    }

    /* Inserts the Value at the specified position */
    FORCEINLINE void InsertAt( SizeType Position, ElementType&& Element ) noexcept
    {
        EmplaceAt( Position, ::Forward<ElementType>( Element ) );
    }

    /* Insert an array into the container at the position */
    inline void InsertAt( SizeType Position, const ElementType* InputArray, SizeType Count ) noexcept
    {
        Assert( Position <= ArraySize );

        /* Special case if at the end */
        if ( Position == ArraySize )
        {
            Append( InputArray, Count );
            return;
        }

        const SizeType NewCapacity = InternalGrowCapacity( ArraySize + Count, ArrayCapacity );
        InternalReserveForInsertion( NewCapacity, Position, Count );
        CopyConstructRange<ElementType>( Data() + Position, InputArray, Count );
    }

    /* Insert an std::initializer_list into the container at the position */
    FORCEINLINE void InsertAt( SizeType Position, std::initializer_list<ElementType> InitList ) noexcept
    {
        InsertAt( Position, InitList.begin(), InitList.size() );
    }

    /* Inserts an array at the end */
    FORCEINLINE void Append( const ElementType* InputArray, SizeType Count ) noexcept
    {
        Assert( InputArray != nullptr );

        const SizeType NewSize = ArraySize + Count;
        if ( NewSize >= ArrayCapacity )
        {
            const SizeType NewCapacity = InternalGrowCapacity( ArraySize + Count, ArrayCapacity );
            InternalReserve( NewCapacity );
        }

        CopyConstructRange<ElementType>( Data() + ArraySize, InputArray, Count );
        ArraySize = NewSize;
    }

    /* Inserts an array at the end */
    FORCEINLINE void Append( const TArray& Other ) noexcept
    {
        Append( Other.Data(), Other.Size() );
    }

    /* Inserts an array at the end */
    FORCEINLINE void Append( std::initializer_list<ElementType> InitList ) noexcept
    {
        Append( InitList.begin(), InitList.size() );
    }

    /* Removes a number of elments from the back */
    FORCEINLINE void PopBackNum( SizeType Count ) noexcept
    {
        if ( !IsEmpty() )
        {
            ArraySize = ArraySize - Count;
            DestructRange<ElementType>( Data() + ArraySize, Count );
        }
    }

    /* Removes the last element */
    FORCEINLINE void PopBack() noexcept
    {
        if ( !IsEmpty() )
        {
            ArraySize--;
            Destruct<ElementType>( Data() + ArraySize );
        }
    }

    /* Removes the element at the position */
    FORCEINLINE void RemoveAt( SizeType Position ) noexcept
    {
        Assert( Position < ArraySize );

        if ( Position == ArraySize - 1 )
        {
            PopBack();
            return;
        }

        Destruct<ElementType>( Data() + Position );
        RelocateRange<ElementType>( Data() + Position, Data() + Position + 1, ArraySize - Position );
        ArraySize--;
    }

    /* Remove a range starting at position and containg count number of elements */
    FORCEINLINE void RemoveRange( SizeType Position, SizeType Count ) noexcept
    {
        Assert( Position + Count < ArraySize );

        if ( Position + Count == ArraySize - 1 )
        {
            PopBackNum( Count );
            return;
        }

        DestructRange<ElementType>( Data() + Position, Count );
        RelocateRange<ElementType>( Data() + Position, Data() + Position + Count, ArraySize - Position );
        ArraySize = ArraySize - Count;
    }

    /* Swaps container with another */
    FORCEINLINE void Swap( TArray& Other ) noexcept
    {
        TArray TempArray( ::Move( *this ) );
        *this = ::Move( Other );
        Other = ::Move( TempArray );
    }

    /* Shrinks the allocation to be the same as the size */
    FORCEINLINE void ShrinkToFit() noexcept
    {
        if ( ArraySize < ArrayCapacity )
        {
            InternalReserve( ArraySize );
        }
    }

    /* Checks if there are any elements */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ArraySize == 0);
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE Iterator StartIterator() noexcept
    {
        return Iterator( Array );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE Iterator EndIterator() noexcept
    {
        return Iterator( Array + ArraySize );
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE ConstIterator StartIterator() const noexcept
    {
        return ConstIterator( Array );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE ConstIterator EndIterator() const noexcept
    {
        return ConstIterator( Array + ArraySize );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseIterator ReverseStartIterator() noexcept
    {
        return ReverseIterator( Array + ArraySize );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseIterator ReverseEndIterator() noexcept
    {
        return ReverseIterator( Array );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ConstReverseIterator ReverseStartIterator() const noexcept
    {
        return ConstReverseIterator( Array + ArraySize );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ConstReverseIterator ReverseEndIterator() const noexcept
    {
        return ConstReverseIterator( Array );
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

    /* Sets the container to another array by copying it */
    FORCEINLINE TArray& operator=( const TArray& Other ) noexcept
    {
        Reset( Other );
        return *this;
    }

    /* Sets the container to another array by moving it */
    FORCEINLINE TArray& operator=( TArray&& Other ) noexcept
    {
        Reset( Other );
        return *this;
    }

    /* Sets the container to a std::initializer_list */
    FORCEINLINE TArray& operator=( std::initializer_list<ElementType> InitList ) noexcept
    {
        Reset( InitList );
        return *this;
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    FORCEINLINE bool operator==( const TArray& Other ) const noexcept
    {
        if ( Size() != Other.Size() )
        {
            return false;
        }

        for ( SizeType i = 0; i < Size(); i++ )
        {
            if ( At( i ) != Other.At( i ) )
            {
                return false;
            }
        }

        return true;
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    FORCEINLINE bool operator!=( const TArray& Other ) const noexcept
    {
        return !(*this == Other);
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
    FORCEINLINE Iterator begin() noexcept
    {
        return Array;
    }

    FORCEINLINE Iterator end() noexcept
    {
        return Array + ArraySize;
    }

    FORCEINLINE ConstIterator begin() const noexcept
    {
        return Array;
    }

    FORCEINLINE ConstIterator end() const noexcept
    {
        return Array + ArraySize;
    }

private:
    FORCEINLINE void InternalConstruct( SizeType Count )
    {
        ElementType* Pointer = Allocator.AllocateOrRealloc( Count );
        DefaultConstructRange<ElementType>( Pointer, Count );
        ArraySize = Count;
        ArrayCapacity = Count;
    }

    FORCEINLINE void InternalConstructFrom( SizeType Count, const ElementType& Element )
    {
        ElementType* Pointer = Allocator.AllocateOrRealloc( Count );
        ConstructRangeFrom<ElementType>( Pointer, Element, Count );
        ArraySize = Count;
        ArrayCapacity = Count;
    }

    FORCEINLINE void InternalCopyFrom( const ElementType* From, SizeType Count )
    {
        ElementType* Pointer = Allocator.AllocateOrRealloc( Count );
        CopyConstructRange<ElementType>( Pointer, From, Count );
        ArraySize = Count;
        ArrayCapacity = Count;
    }

    FORCEINLINE void InternalMoveFrom( TArray&& FromArray )
    {
        // Since the memory remains the same we should not need to use move-assignment or constructor
        Allocator.MoveFrom( FromArray.Allocator );
        ArraySize = FromArray.ArraySize;
        ArrayCapacity = FromArray.ArrayCapacity;
        FromArray.ArraySize = 0;
        FromArray.ArrayCapacity = 0;
    }

    FORCEINLINE void InternalReserve( const SizeType NewCapacity ) noexcept
    {
        /* Simple Memory::Realloc for trivial elements */
        if constexpr ( !TIsTrivial<ElementType>::Value )
        {
            /*
            * For non-trivial objects a temporary allocator has to be created since the old memory needs to be saved so
            * that the objects can properly be relocated. Example is when objects has pointers/references to themselves,
            * or contains childobjects that needs to be updated etc. The allocator itself cannot do this, since it does
            * not know if objects has been created for a certain location or not.
            */

            AllocatorType NewAllocator;
            NewAllocator.AllocateOrRealloc( NewCapacity );
            RelocateRange<ElementType>( NewAllocator.Raw(), Data(), ArraySize );
            Allocator.MoveFrom( NewAllocator );
        }
        else
        {
            Allocator.AllocateOrRealloc( NewCapacity );
        }

        ArrayCapacity = NewCapacity;
    }

    FORCEINLINE void InternalReserveForInsertion( const SizeType NewCapacity, const SizeType InsertAt, const SizeType ElementCount ) noexcept
    {
        Assert( NewCapacity >= ArrayCapacity );

        if ( NewCapacity > ArrayCapacity )
        {
            /* Simpler path for trivial elements */
            if constexpr ( !TIsTrivial<ElementType>::Value )
            {
                /*
                * For non-trivial objects a temporary allocator has to be created since the old memory needs to be saved so
                * that the objects can properly be relocated. Example is when objects has pointers/references to themselves,
                * or contains childobjects that needs to be updated etc. The allocator itself cannot do this, since it does
                * not know if objects has been created for a certain location or not.
                */

                AllocatorType NewAllocator;
                NewAllocator.AllocateOrRealloc( NewCapacity );
                /* Elements before new area */
                RelocateRange<ElementType>( NewAllocator.Raw(), Data(), InsertAt );
                /* Elements after new area */
                RelocateRange<ElementType>( NewAllocator.Raw() + InsertAt, Data() + InsertAt + ElementCount, ArraySize - InsertAt );
                Allocator.MoveFrom( NewAllocator );
            }
            else
            {
                Allocator.AllocateOrRealloc( NewCapacity );
                /* Elements after new area */
                RelocateRange<ElementType>( Data() + InsertAt, Data() + InsertAt + ElementCount, ArraySize - InsertAt );
            }

            ArrayCapacity = NewCapacity;
        }
        else
        {
            /* Elements after new area */
            RelocateRange<ElementType>( Data() + InsertAt, Data() + InsertAt + ElementCount, ArraySize - InsertAt );
        }
    }

    FORCEINLINE static SizeType InternalGrowCapacity( SizeType NewSize, SizeType CurrentCapacity ) noexcept
    {
        return NewSize + (CurrentCapacity / 2);
    }

private:
    /* Allocator contains the pointer */
    AllocatorType Allocator;
    SizeType ArraySize;
    SizeType ArrayCapacity;
};
