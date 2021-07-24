#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"

/* TArrayView - View of an array similar to std::span */
template<typename T>
class TArrayView
{
public:
    typedef T                                   ElementType;
    typedef ElementType*                        IteratorType;
    typedef const ElementType*                  ConstIteratorType;
    typedef TReverseIterator<ElementType>       ReverseIteratorType;
    typedef TReverseIterator<const ElementType> ConstReverseIteratorType;
    typedef uint32                              SizeType;

    /* Default construct an empty view */
    FORCEINLINE TArrayView() noexcept
        : View( nullptr )
        , ViewSize( 0 )
    {
    }

    /* Create a view from a templated array type */
    template<typename ArrayType>
    FORCEINLINE explicit TArrayView( ArrayType& InArray ) noexcept
        : View( InArray.Data() )
        , ViewSize( InArray.Size() )
    {
    }

    /* Create a view from a bounded array */
    template<const SizeType N>
    FORCEINLINE explicit TArrayView( ElementType( &InArray )[N] ) noexcept
        : View( InArray )
        , ViewSize( N )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE explicit TArrayView(const ElementType* InArray, SizeType Count ) noexcept
        : View( InArray )
        , ViewSize( Count )
    {
    }

    /* Create a view from another view */
    FORCEINLINE TArrayView( const TArrayView& Other ) noexcept
        : View( Other.View )
        , ViewSize( Other.ViewSize )
    {
    }

    /* Move anther view into this one */
    FORCEINLINE TArrayView( TArrayView&& Other ) noexcept
        : View( Other.View )
        , ViewSize( Other.ViewSize )
    {
        Other.View = nullptr;
        Other.ViewSize = 0;
    }

    /* Check if the size is zero or not */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ViewSize == 0) && (Data() != nullptr);
    }

    /* Retrive the first element */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert( IsEmpty() );
        return Data()[0];
    }

    /* Retrive the first element */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[0];
    }

    /* Retrive the last element */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert( IsEmpty() );
        return Data()[ViewSize - 1];
    }

    /* Retrive the last element */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[ViewSize - 1];
    }

    /* Retrive an element at a certain position */
    FORCEINLINE ElementType& At( SizeType Index ) noexcept
    {
        Assert( Index < ViewSize );
        return Data()[Index];
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const ElementType& At( SizeType Index ) const noexcept
    {
        Assert( Index < ViewSize );
        return Data()[Index];
    }

    /* Swap two views */
    FORCEINLINE void Swap( TArrayView& Other ) noexcept
    {
        TArrayView Temp( ::Move( *this ) );
        *this = ::Move( Other );
        Other = ::Move( Temp );
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType( Data() );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType( Data() + Size() );
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType( Data() );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType( Data() + Size() );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType( Data() + Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType( Data() );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ConstReverseIteratorType ReverseStartIterator() const noexcept
    {
        return ConstReverseIteratorType( Data() + Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ConstReverseIteratorType ReverseEndIterator() const noexcept
    {
        return ConstReverseIteratorType( Data() );
    }

    /* Fills the container with the specified value */
    template<typename FillType>
    FORCEINLINE typename TEnableIf<TIsAssignable<T, typename TAddLeftReference<const FillType>::Type>::Value>::Type Fill( const FillType& InputElement ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = InputElement;
        }
    }

    /* Fills the container with the specified value */
    template<typename FillType>
    FORCEINLINE typename TEnableIf<TIsAssignable<T, typename TAddRightReference<FillType>::Type>::Value>::Type Fill( FillType&& InputElement ) noexcept
    {
        for ( ElementType& Element : *this )
        {
            Element = ::Move( InputElement );
        }
    }

    /* Retrive the last valid index for the view */
    FORCEINLINE SizeType LastIndex() const noexcept
    {
        return ViewSize > 0 ? ViewSize - 1 : 0;
    }

    /* Retrive the size of the view */ 
    FORCEINLINE SizeType Size() const noexcept
    {
        return ViewSize;
    }

    /* Retrive the size of the view in bytes */ 
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return ViewSize * sizeof( ElementType );
    }

    /* Retrive the data of the view */ 
    FORCEINLINE lementType* Data() noexcept
    {
        return View;
    }

    /* Retrive the data of the view */ 
    FORCEINLINE const ElementType* Data() const noexcept
    {
        return View;
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename ArrayType>
    FORCEINLINE bool operator==( const ArrayType& Other ) const noexcept
    {
        if ( Size() != Other.Size() )
        {
            return false;
        }

        return CompareRange<ElementType>(Data(), Other.Data(), Size());
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename ArrayType>
    FORCEINLINE bool operator!=( const ArrayType& Other ) const noexcept
    {
        return !(*this == Other);
    }

    /* Retrive an element at a certain position */
    FORCEINLINE ElementType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const ElementType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    /* Assign from another view */
    FORCEINLINE TArrayView& operator=( const TArrayView& Other ) noexcept
    {
        View = Other.View;
        ViewSize = Other.ViewSize;
        return *this;
    }

    /* Move-assign from another view */
    FORCEINLINE TArrayView& operator=( TArrayView&& Other ) noexcept
    {
        if ( this != &Other )
        {
            View = Other.View;
            ViewSize = Other.ViewSize;
            Other.View = nullptr;
            Other.ViewSize = 0;
        }

        return *this;
    }

public:

    /* STL iterator functions - Enables Range-based for-loops */

    FORCEINLINE IteratorType begin() noexcept
    {
        return View;
    }

    FORCEINLINE IteratorType end() noexcept
    {
        return View + ViewSize;
    }

    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return View;
    }

    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return View + ViewSize;
    }

private:
    ElementType* View;
    SizeType ViewSize;
};
