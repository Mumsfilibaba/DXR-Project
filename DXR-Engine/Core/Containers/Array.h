#pragma once
#include "Iterator.h"
#include "Allocator.h"

#include "Core/Templates/Move.h"
#include "Core/Templates/IsPointer.h"
#include "Core/Templates/IsSame.h"

#include <initializer_list>

/* Dynamic Array similar to std::vector */
template<typename T, typename TAllocator = TDefaultAllocator<T>>
class TArray
{
public:
    typedef T                                   ElementType;
    typedef ElementType*                        Iterator;
    typedef const ElementType*                  ConstIterator;
    typedef TReverseIterator<ElementType>       ReverseIterator;
    typedef TReverseIterator<const ElementType> ConstReverseIterator;
    typedef uint32                              SizeType;

    FORCEINLINE TArray() noexcept
        : Array( nullptr )
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
        , Allocator()
    {
    }

    FORCEINLINE explicit TArray( SizeType Size ) noexcept
        : Array( nullptr )
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
        , Allocator()
    {
        InternalConstruct( Size );
    }

    FORCEINLINE explicit TArray( SizeType Size, const ElementType& Value ) noexcept
        : Array( nullptr )
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
        , Allocator()
    {
        InternalConstruct( Size, Value );
    }

    FORCEINLINE TArray( ElementType* Array, SizeType Count ) noexcept
        : Array( nullptr )
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
        , Allocator()
    {
        // TODO: Contruct from array here
    }

    FORCEINLINE TArray( std::initializer_list<T> List ) noexcept
        : Array( nullptr )
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
        , Allocator()
    {
        InternalConstruct( List.begin(), List.end() );
    }

    FORCEINLINE TArray( const TArray& Other ) noexcept
        : Array( nullptr )
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
        , Allocator()
    {
        InternalConstruct( Other.Begin(), Other.End() );
    }

    FORCEINLINE TArray( TArray&& Other ) noexcept
        : Array( nullptr )
        , ArraySize( 0 )
        , ArrayCapacity( 0 )
        , Allocator()
    {
        InternalMove( ::Forward<TArray>( Other ) );
    }

    FORCEINLINE ~TArray()
    {
        Clear();
        InternalReleaseData();
        ArrayCapacity = 0;
    }

    FORCEINLINE void Clear() noexcept
    {
        InternalDestructRange( Array, Array + ArraySize );
        ArraySize = 0;
    }

    FORCEINLINE void Assign( SizeType Size ) noexcept
    {
        Clear();
        InternalConstruct( Size );
    }

    FORCEINLINE void Assign( SizeType Size, const T& Value ) noexcept
    {
        Clear();
        InternalConstruct( Size, Value );
    }

    template<typename TInput>
    FORCEINLINE void Assign( TInput Begin, TInput End ) noexcept
    {
        Clear();
        InternalConstruct( Begin, End );
    }

    FORCEINLINE void Assign( std::initializer_list<T> List ) noexcept
    {
        Clear();
        InternalConstruct( List.begin(), List.end() );
    }

    FORCEINLINE void Fill( const T& Value ) noexcept
    {
        T* ArrayBegin = Array;
        T* ArrayEnd = ArrayBegin + ArraySize;

        while ( ArrayBegin != ArrayEnd )
        {
            *ArrayBegin = Value;
            ArrayBegin++;
        }
    }

    FORCEINLINE void Fill( T&& Value ) noexcept
    {
        T* ArrayBegin = Array;
        T* ArrayEnd = ArrayBegin + ArraySize;

        while ( ArrayBegin != ArrayEnd )
        {
            *ArrayBegin = ::Move( Value );
            ArrayBegin++;
        }
    }

    FORCEINLINE void Resize( SizeType InSize ) noexcept
    {
        if ( InSize > ArraySize )
        {
            if ( InSize > ArrayCapacity )
            {
                InternalRealloc( InSize );
            }

            InternalDefaultConstructRange( Array + ArraySize, Array + InSize );
        }
        else if ( InSize < ArraySize )
        {
            InternalDestructRange( Array + InSize, Array + ArraySize );
        }

        ArraySize = InSize;
    }

    FORCEINLINE void Resize( SizeType InSize, const T& Value ) noexcept
    {
        if ( InSize > ArraySize )
        {
            if ( InSize > ArrayCapacity )
            {
                InternalRealloc( InSize );
            }

            InternalCopyEmplace( InSize - ArraySize, Value, Array + ArraySize );
        }
        else if ( InSize < ArraySize )
        {
            InternalDestructRange( Array + InSize, Array + ArraySize );
        }

        ArraySize = InSize;
    }

    FORCEINLINE void Reserve( SizeType Capacity ) noexcept
    {
        if ( Capacity != ArrayCapacity )
        {
            SizeType OldSize = ArraySize;
            if ( Capacity < ArraySize )
            {
                ArraySize = Capacity;
            }

            T* TempData = InternalAllocateElements( Capacity );
            InternalMoveEmplace( Array, Array + ArraySize, TempData );
            InternalDestructRange( Array, Array + OldSize );

            InternalReleaseData();
            Array = TempData;
            ArrayCapacity = Capacity;
        }
    }

    template<typename... TArgs>
    FORCEINLINE T& EmplaceBack( TArgs&&... Args ) noexcept
    {
        if ( ArraySize >= ArrayCapacity )
        {
            const SizeType NewCapacity = InternalGetResizeFactor();
            InternalRealloc( NewCapacity );
        }

        T* DataEnd = Array + ArraySize;
        new(reinterpret_cast<void*>(DataEnd)) T( ::Forward<TArgs>( Args )... );
        ArraySize++;
        return (*DataEnd);
    }

    FORCEINLINE T& PushBack( const T& Element ) noexcept
    {
        return EmplaceBack( Element );
    }

    FORCEINLINE T& PushBack( T&& Element ) noexcept
    {
        return EmplaceBack( Move( Element ) );
    }

    template<typename... TArgs>
    inline Iterator Emplace( ConstIterator Pos, TArgs&&... Args ) noexcept
    {
        Assert( InternalIsIteratorOwner( Pos ) );

        if ( Pos == End() )
        {
            const SizeType OldSize = ArraySize;
            EmplaceBack( ::Forward<TArgs>( Args )... );
            return End() - 1;
        }

        const SizeType Index = InternalIndex( Pos );
        T* DataBegin = Array + Index;
        if ( ArraySize >= ArrayCapacity )
        {
            const SizeType NewCapacity = InternalGetResizeFactor();
            InternalEmplaceRealloc( NewCapacity, DataBegin, 1 );
            DataBegin = Array + Index;
        }
        else
        {
            // Construct the range so that we can move to It
            T* DataEnd = Array + ArraySize;
            InternalDefaultConstructRange( DataEnd, DataEnd + 1 );
            InternalMemmoveForward( DataBegin, DataEnd, DataEnd );
            InternalDestruct( DataBegin );
        }

        new (reinterpret_cast<void*>(DataBegin)) T( ::Forward<TArgs>( Args )... );
        ArraySize++;
        return Iterator( DataBegin );
    }

    FORCEINLINE Iterator Insert( Iterator Pos, const T& Value ) noexcept
    {
        return Emplace( Pos, Value );
    }

    FORCEINLINE Iterator Insert( Iterator Pos, T&& Value ) noexcept
    {
        return Emplace( Pos, Move( Value ) );
    }

    FORCEINLINE Iterator Insert( ConstIterator Pos, const T& Value ) noexcept
    {
        return Emplace( Pos, Value );
    }

    FORCEINLINE Iterator Insert( ConstIterator Pos, T&& Value ) noexcept
    {
        return Emplace( Pos, Move( Value ) );
    }

    FORCEINLINE Iterator Insert( Iterator Pos, std::initializer_list<T> List ) noexcept
    {
        return Insert( ConstIterator( Pos ), List );
    }

    inline Iterator Insert( ConstIterator Pos, std::initializer_list<T> List ) noexcept
    {
        Assert( InternalIsIteratorOwner( Pos ) );

        if ( Pos == End() )
        {
            for ( const T& Value : List )
            {
                EmplaceBack( Move( Value ) );
            }

            return End() - 1;
        }

        const SizeType ListSize = static_cast<SizeType>(List.size());
        const SizeType NewSize = ArraySize + ListSize;
        const SizeType Index = InternalIndex( Pos );

        T* RangeBegin = Array + Index;
        if ( NewSize >= ArrayCapacity )
        {
            const SizeType NewCapacity = InternalGetResizeFactor( NewSize );
            InternalEmplaceRealloc( NewCapacity, RangeBegin, ListSize );
            RangeBegin = Array + Index;
        }
        else
        {
            // Construct the range so that we can move to It
            T* DataEnd = Array + ArraySize;
            T* NewDataEnd = Array + ArraySize + ListSize;
            T* RangeEnd = RangeBegin + ListSize;
            InternalDefaultConstructRange( DataEnd, NewDataEnd );
            InternalMemmoveForward( RangeBegin, DataEnd, NewDataEnd - 1 );
            InternalDestructRange( RangeBegin, RangeEnd );
        }

        // TODO: Get rid of const_cast
        InternalMoveEmplace( const_cast<T*>(List.begin()), const_cast<T*>(List.end()), RangeBegin );
        ArraySize = NewSize;
        return Iterator( RangeBegin );
    }

    template<typename TInput>
    FORCEINLINE Iterator Insert( Iterator Pos, TInput Begin, TInput End ) noexcept
    {
        return Insert( Pos, Begin, End );
    }

    template<typename TInput>
    inline Iterator Insert( ConstIterator Pos, TInput InBegin, TInput InEnd ) noexcept
    {
        Assert( InternalIsIteratorOwner( Pos ) );

        if ( Pos == End() )
        {
            for ( TInput It = InBegin; It != InEnd; It++ )
            {
                EmplaceBack( *It );
            }

            return End() - 1;
        }

        const SizeType RangeSize = InternalDistance( InBegin, InEnd );
        const SizeType NewSize = ArraySize + RangeSize;
        const SizeType Index = InternalIndex( Pos );

        T* RangeBegin = Array + Index;
        if ( NewSize >= ArrayCapacity )
        {
            const SizeType NewCapacity = InternalGetResizeFactor( NewSize );
            InternalEmplaceRealloc( NewCapacity, RangeBegin, RangeSize );
            RangeBegin = Array + Index;
        }
        else
        {
            // Construct the range so that we can move to it
            T* DataEnd = Array + ArraySize;
            T* NewDataEnd = Array + ArraySize + RangeSize;
            T* RangeEnd = RangeBegin + RangeSize;
            InternalDefaultConstructRange( DataEnd, NewDataEnd );
            InternalMemmoveForward( RangeBegin, DataEnd, NewDataEnd - 1 );
            InternalDestructRange( RangeBegin, RangeEnd );
        }

        InternalCopyEmplace( Begin, End, RangeBegin );
        ArraySize = NewSize;
        return Iterator( RangeBegin );
    }

    FORCEINLINE Iterator Append( const TArray& Other )
    {
        return Insert( End(), Other.Begin(), Other.End() );
    }

    FORCEINLINE void PopBack() noexcept
    {
        if ( !IsEmpty() )
        {
            InternalDestruct( Array + (--ArraySize) );
        }
    }

    FORCEINLINE Iterator Erase( Iterator Pos ) noexcept
    {
        return Erase( ConstIterator( Pos ) );
    }

    FORCEINLINE Iterator Erase( ConstIterator Pos ) noexcept
    {
        Assert( InternalIsIteratorOwner( Pos ) );
        Assert( Pos < End() );

        if ( Pos == End() - 1 )
        {
            PopBack();
            return End();
        }

        const SizeType Index = InternalIndex( Pos );
        T* DataBegin = Array + Index;
        T* DataEnd = Array + ArraySize;
        InternalMemmoveBackwards( DataBegin + 1, DataEnd, DataBegin );
        InternalDestruct( DataEnd - 1 );

        ArraySize--;
        return Iterator( DataBegin );
    }

    FORCEINLINE Iterator Erase( Iterator Begin, Iterator End ) noexcept
    {
        return Erase( ConstIterator( Begin ), ConstIterator( End ) );
    }

    inline Iterator Erase( ConstIterator InBegin, ConstIterator InEnd ) noexcept
    {
        Assert( InBegin < InEnd );
        Assert( InternalIsRangeOwner( InBegin, InEnd ) );

        T* DataBegin = Array + InternalIndex( InBegin );
        T* DataEnd = Array + InternalIndex( InEnd );

        const SizeType elementCount = InternalDistance( DataBegin, DataEnd );
        if ( InEnd == End() )
        {
            InternalDestructRange( DataBegin, DataEnd );
        }
        else
        {
            T* RealEnd = Array + ArraySize;
            InternalMemmoveBackwards( DataEnd, RealEnd, DataBegin );
            InternalDestructRange( RealEnd - elementCount, RealEnd );
        }

        ArraySize -= elementCount;
        return Iterator( DataBegin );
    }

    FORCEINLINE void Swap( TArray& Other ) noexcept
    {
        TArray TempArray( ::Move( *this ) );
        *this = ::Move( Other );
        Other = ::Move( TempArray );
    }

    FORCEINLINE void ShrinkToFit() noexcept
    {
        if ( ArrayCapacity > ArraySize )
        {
            InternalRealloc( ArraySize );
        }
    }

    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ArraySize == 0);
    }

    FORCEINLINE T& Front() noexcept
    {
        Assert( !IsEmpty() );
        return Array[0];
    }

    FORCEINLINE Iterator Begin() noexcept
    {
        return Iterator( Array );
    }

    FORCEINLINE Iterator End() noexcept
    {
        return Iterator( Array + ArraySize );
    }

    FORCEINLINE ConstIterator Begin() const noexcept
    {
        return Iterator( Array );
    }

    FORCEINLINE ConstIterator End() const noexcept
    {
        return Iterator( Array + ArraySize );
    }

    FORCEINLINE const T& Front() const noexcept
    {
        Assert( ArraySize > 0 );
        return Array[0];
    }

    FORCEINLINE T& Back() noexcept
    {
        Assert( ArraySize > 0 );
        return Array[ArraySize - 1];
    }

    FORCEINLINE const T& Back() const noexcept
    {
        Assert( ArraySize > 0 );
        return Array[ArraySize - 1];
    }

    FORCEINLINE T* Data() noexcept
    {
        return Array;
    }

    FORCEINLINE const T* Data() const noexcept
    {
        return Array;
    }

    FORCEINLINE SizeType LastIndex() const noexcept
    {
        return ArraySize > 0 ? ArraySize - 1 : 0;
    }

    FORCEINLINE SizeType Size() const noexcept
    {
        return ArraySize;
    }
    
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return ArraySize * Allocator.StrideInBytes();
    }

    FORCEINLINE SizeType Capacity() const noexcept
    {
        return ArrayCapacity;
    }

    FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return ArrayCapacity * Allocator.StrideInBytes();
    }

    FORCEINLINE T& At( SizeType Index ) noexcept
    {
        Assert( Index < ArraySize );
        return Array[Index];
    }

    FORCEINLINE const T& At( SizeType Index ) const noexcept
    {
        Assert( Index < ArraySize );
        return Array[Index];
    }

    FORCEINLINE TArray& operator=( const TArray& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Clear();
            InternalConstruct( Other.Begin(), Other.End() );
        }

        return *this;
    }

    FORCEINLINE TArray& operator=( TArray&& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Clear();
            InternalMove( ::Forward<TArray>( Other ) );
        }

        return *this;
    }

    FORCEINLINE TArray& operator=( std::initializer_list<T> List ) noexcept
    {
        Assign( List );
        return *this;
    }

    FORCEINLINE T& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    FORCEINLINE const T& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    // STL iterator functions - Enables Range-based for-loops
public:
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

    FORCEINLINE ConstIterator cbegin() const noexcept
    {
        return Array;
    }

    FORCEINLINE ConstIterator cend() const noexcept
    {
        return Array + ArraySize;
    }

    FORCEINLINE ReverseIterator rbegin() noexcept
    {
        return ReverseIterator( end() );
    }

    FORCEINLINE ReverseIterator rend() noexcept
    {
        return ReverseIterator( begin() );
    }

    FORCEINLINE ConstReverseIterator rbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }

    FORCEINLINE ConstReverseIterator rend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

    FORCEINLINE ConstReverseIterator crbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }

    FORCEINLINE ConstReverseIterator crend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

private:

    // Check that the iterator belongs to this TArray
    FORCEINLINE bool InternalIsRangeOwner( ConstIterator InBegin, ConstIterator InEnd ) const noexcept
    {
        return (InBegin < InEnd) && (InBegin >= Begin()) && (InEnd <= End());
    }

    FORCEINLINE bool InternalIsIteratorOwner( ConstIterator It ) const noexcept
    {
        return (It >= Begin()) && (It <= End());
    }

    // Helpers
    template<typename TInput>
    FORCEINLINE const T* InternalUnwrapConst( TInput It ) noexcept
    {
        if constexpr ( IsPointer<TInput> )
        {
            return It;
        }
        else
        {
            return It.Ptr;
        }
    }

    template<typename TInput>
    FORCEINLINE SizeType InternalDistance( TInput InBegin, TInput InEnd ) noexcept
    {
        constexpr bool TypeIsPointer = IsPointer<TInput>;
        constexpr bool TypeIsCustomIterator = IsSame<TInput, Iterator> || IsSame<TInput, ConstIterator>;

        // Handle outside pointers
        if constexpr ( TypeIsPointer || TypeIsCustomIterator )
        {
            return static_cast<SizeType>(InternalUnwrapConst( InEnd ) - InternalUnwrapConst( InBegin ));
        }
        else
        {
            // TODO: Custom std::distance?
            return static_cast<SizeType>(std::distance( InBegin, InEnd ));
        }
    }

    template<typename TInput>
    FORCEINLINE SizeType InternalIndex( TInput Pos ) noexcept
    {
        return static_cast<SizeType>(InternalUnwrapConst( Pos ) - InternalUnwrapConst( begin() ));
    }

    FORCEINLINE SizeType InternalGetResizeFactor() const noexcept
    {
        return InternalGetResizeFactor( ArraySize );
    }

    FORCEINLINE SizeType InternalGetResizeFactor( SizeType BaseSize ) const noexcept
    {
        return BaseSize + (ArrayCapacity / 2) + 1;
    }

    FORCEINLINE T* InternalAllocateElements( SizeType Capacity ) noexcept
    {
        // TODO: Why have this in a function
        return Allocator.Allocate( Capacity );
    }

    FORCEINLINE void InternalReleaseData() noexcept
    {
        if ( Array )
        {
            Allocator.Free( Array );
            Array = nullptr;
        }
    }

    FORCEINLINE void InternalAllocData( SizeType Capacity ) noexcept
    {
        if ( Capacity > ArrayCapacity )
        {
            InternalReleaseData();
            Array = InternalAllocateElements( Capacity );
            ArrayCapacity = Capacity;
        }
    }

    FORCEINLINE void InternalRealloc( SizeType Capacity ) noexcept
    {
        T* TempData = InternalAllocateElements( Capacity );
        InternalMoveEmplace( Array, Array + ArraySize, TempData );
        InternalDestructRange( Array, Array + ArraySize );

        InternalReleaseData();
        Array = TempData;
        ArrayCapacity = Capacity;
    }

    FORCEINLINE void InternalEmplaceRealloc( SizeType Capacity, T* EmplacePos, SizeType Count ) noexcept
    {
        Assert( Capacity >= ArraySize + Count );

        const SizeType Index = InternalIndex( EmplacePos );
        T* TempData = InternalAllocateElements( Capacity );
        InternalMoveEmplace( Array, EmplacePos, TempData );
        if ( EmplacePos != Array + ArraySize )
        {
            InternalMoveEmplace( EmplacePos, Array + ArraySize, TempData + Index + Count );
        }

        InternalDestructRange( Array, Array + ArraySize );

        InternalReleaseData();
        Array = TempData;
        ArrayCapacity = Capacity;
    }

    // Construct
    FORCEINLINE void InternalConstruct( SizeType InSize ) noexcept
    {
        if ( InSize > 0 )
        {
            InternalAllocData( InSize );
            ArraySize = InSize;
            InternalDefaultConstructRange( Array, Array + InSize );
        }
    }

    FORCEINLINE void InternalConstruct( SizeType InSize, const T& Value ) noexcept
    {
        if ( InSize > 0 )
        {
            InternalAllocData( InSize );
            InternalCopyEmplace( InSize, Value, Array );
            ArraySize = InSize;
        }
    }

    template<typename TInput>
    FORCEINLINE void InternalConstruct( TInput InBegin, TInput InEnd ) noexcept
    {
        const SizeType Distance = InternalDistance( InBegin, InEnd );
        if ( Distance > 0 )
        {
            InternalAllocData( Distance );
            InternalCopyEmplace( InBegin, InEnd, Array );
            ArraySize = Distance;
        }
    }

    FORCEINLINE void InternalMove( TArray&& Other ) noexcept
    {
        InternalReleaseData();

        Array = Other.Array;
        ArraySize = Other.ArraySize;
        ArrayCapacity = Other.ArrayCapacity;

        Other.Array = nullptr;
        Other.ArraySize = 0;
        Other.ArrayCapacity = 0;
    }

    // Emplace
    template<typename TInput>
    FORCEINLINE void InternalCopyEmplace( TInput Begin, TInput End, T* Dest ) noexcept
    {
        // This function assumes that there is no overlap
        constexpr bool TypeIsTrivial = std::is_trivially_copy_constructible<T>(); // TODO: Make custom version?
        constexpr bool TypeIsPointer = IsPointer<TInput>;
        constexpr bool TypeIsCustomIterator = IsSame<TInput, Iterator> || IsSame<TInput, ConstIterator>;

        if constexpr ( TypeIsTrivial && (TypeIsPointer || TypeIsCustomIterator) )
        {
            const SizeType Count = InternalDistance( Begin, End );
            const SizeType CpySize = Count * sizeof( T );
            memcpy( Dest, InternalUnwrapConst( Begin ), CpySize );
        }
        else
        {
            while ( Begin != End )
            {
                new(reinterpret_cast<void*>(Dest)) T( *Begin );
                Begin++;
                Dest++;
            }
        }
    }

    FORCEINLINE void InternalCopyEmplace( SizeType Size, const T& Value, T* Dest ) noexcept
    {
        T* ItEnd = Dest + Size;
        while ( Dest != ItEnd )
        {
            new(reinterpret_cast<void*>(Dest)) T( Value );
            Dest++;
        }
    }

    FORCEINLINE void InternalMoveEmplace( T* InBegin, T* InEnd, T* Dest ) noexcept
    {
        // This function assumes that there is no overlap
        if constexpr ( std::is_trivially_move_constructible<T>() )
        {
            const SizeType Count = InternalDistance( InBegin, InEnd );
            const SizeType CpySize = Count * sizeof( T );
            ::memcpy( Dest, InBegin, CpySize );
        }
        else
        {
            while ( InBegin != InEnd )
            {
                new(reinterpret_cast<void*>(Dest)) T( ::Move( *InBegin ) );
                InBegin++;
                Dest++;
            }
        }
    }

    inline void InternalMemmoveBackwards( T* InBegin, T* InEnd, T* Dest ) noexcept
    {
        Assert( InBegin <= InEnd );
        if ( InBegin == InEnd )
        {
            return;
        }

        Assert( InEnd <= Array + ArrayCapacity );

        // Move each object in the range to the destination
        const SizeType Count = InternalDistance( InBegin, InEnd );
        if constexpr ( std::is_trivially_move_assignable<T>() )
        {
            const SizeType CpySize = Count * sizeof( T );
            ::memmove( Dest, InBegin, CpySize ); // Assumes that data can overlap
        }
        else
        {
            while ( InBegin != InEnd )
            {
                if constexpr ( std::is_move_assignable<T>() )
                {
                    (*Dest) = Move( *InBegin );
                }
                else if constexpr ( std::is_copy_assignable<T>() )
                {
                    (*Dest) = (*InBegin);
                }

                Dest++;
                InBegin++;
            }
        }
    }

    FORCEINLINE void InternalMemmoveForward( T* InBegin, T* InEnd, T* Dest ) noexcept
    {
        // Move each object in the range to the destination, starts in the "End" and moves forward

        const SizeType Count = InternalDistance( InBegin, InEnd );
        if constexpr ( std::is_trivially_move_assignable<T>() )
        {
            if ( Count > 0 )
            {
                const SizeType CpySize = Count * sizeof( T );
                const SizeType OffsetSize = (Count - 1) * sizeof( T );
                ::memmove( reinterpret_cast<char*>(Dest) - OffsetSize, InBegin, CpySize );
            }
        }
        else
        {
            while ( InEnd != InBegin )
            {
                InEnd--;
                if constexpr ( std::is_move_assignable<T>() )
                {
                    (*Dest) = Move( *InEnd );
                }
                else if constexpr ( std::is_copy_assignable<T>() )
                {
                    (*Dest) = (*InEnd);
                }
                Dest--;
            }
        }
    }

    FORCEINLINE void InternalDestruct( const T* Pos ) noexcept
    {
        if constexpr ( std::is_trivially_destructible<T>() == false )
        {
            (*Pos).~T();
        }
    }

    FORCEINLINE void InternalDestructRange( const T* InBegin, const T* InEnd ) noexcept
    {
        Assert( InBegin <= InEnd );
        Assert( InEnd - InBegin <= ArrayCapacity );

        if constexpr ( std::is_trivially_destructible<T>() == false )
        {
            while ( InBegin != InEnd )
            {
                InternalDestruct( InBegin );
                InBegin++;
            }
        }
    }

    FORCEINLINE void InternalDefaultConstructRange( T* InBegin, T* InEnd ) noexcept
    {
        Assert( InBegin <= InEnd );

        if constexpr ( std::is_default_constructible<T>() )
        {
            while ( InBegin != InEnd )
            {
                new(reinterpret_cast<void*>(InBegin)) T();
                InBegin++;
            }
        }
    }

private:
    T* Array;
    SizeType   ArraySize;
    SizeType   ArrayCapacity;
    TAllocator Allocator;
};
