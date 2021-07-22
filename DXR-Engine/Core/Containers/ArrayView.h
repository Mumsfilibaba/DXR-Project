#pragma once
#include "Iterator.h"

#include "Core/Templates/Move.h"

// TArrayView - View of an array similar to std::span

template<typename T>
class TArrayView
{
public:
    typedef T                                   ElementType;
    typedef ElementType* Iterator;
    typedef const ElementType* ConstIterator;
    typedef TReverseIterator<ElementType>       ReverseIterator;
    typedef TReverseIterator<const ElementType> ConstReverseIterator;
    typedef uint32                              SizeType;

    TArrayView() noexcept
        : View( nullptr )
        , ViewSize( 0 )
    {
    }

    template<typename ArrayType>
    explicit TArrayView( ArrayType& Array ) noexcept
        : View( Array.Data() )
        , ViewSize( Array.Size() )
    {
    }

    template<const SizeType N>
    explicit TArrayView( ElementType( &Array )[N] ) noexcept
        : View( Array )
        , ViewSize( N )
    {
    }

    TArrayView( const TArrayView& Other ) noexcept
        : View( Other.View )
        , ViewSize( Other.ViewSize )
    {
    }

    TArrayView( TArrayView&& Other ) noexcept
        : View( Other.View )
        , ViewSize( Other.ViewSize )
    {
        Other.View = nullptr;
        Other.ViewSize = 0;
    }

    bool IsEmpty() const noexcept
    {
        return (ViewSize == 0);
    }

    ElementType& Front() noexcept
    {
        return View[0];
    }

    const ElementType& Front() const noexcept
    {
        return View[0];
    }

    ElementType& Back() noexcept
    {
        return View[ViewSize - 1];
    }

    const ElementType& Back() const noexcept
    {
        return View[ViewSize - 1];
    }

    ElementType& At( SizeType Index ) noexcept
    {
        Assert( Index < ViewSize );
        return View[Index];
    }

    const ElementType& At( SizeType Index ) const noexcept
    {
        Assert( Index < ViewSize );
        return View[Index];
    }

    void Swap( TArrayView& Other ) noexcept
    {
        TArrayView TempView( ::Move( *this ) );
        *this = ::Move( Other );
        Other = ::Move( TempView );
    }

    Iterator Begin() noexcept
    {
        return Iterator( View );
    }

    Iterator End() noexcept
    {
        return Iterator( View + ViewSize );
    }

    ConstIterator Begin() const noexcept
    {
        return Iterator( View );
    }

    ConstIterator End() const noexcept
    {
        return Iterator( View + ViewSize );
    }

    SizeType LastIndex() const noexcept
    {
        return ViewSize > 0 ? ViewSize - 1 : 0;
    }

    SizeType Size() const noexcept
    {
        return ViewSize;
    }

    SizeType SizeInBytes() const noexcept
    {
        return ViewSize * sizeof( ElementType );
    }

    ElementType* Data() noexcept
    {
        return View;
    }

    const ElementType* Data() const noexcept
    {
        return View;
    }

    ElementType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    const ElementType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    TArrayView& operator=( const TArrayView& Other ) noexcept
    {
        View = Other.View;
        ViewSize = Other.ViewSize;
        return *this;
    }

    TArrayView& operator=( TArrayView&& Other ) noexcept
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
    Iterator begin() noexcept
    {
        return View;
    }

    Iterator end() noexcept
    {
        return View + ViewSize;
    }

    ConstIterator begin() const noexcept
    {
        return View;
    }

    ConstIterator end() const noexcept
    {
        return View + ViewSize;
    }

    ConstIterator cbegin() const noexcept
    {
        return View;
    }

    ConstIterator cend() const noexcept
    {
        return View + ViewSize;
    }

    ReverseIterator rbegin() noexcept
    {
        return ReverseIterator( end() );
    }

    ReverseIterator rend() noexcept
    {
        return ReverseIterator( begin() );
    }

    ConstReverseIterator rbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }

    ConstReverseIterator rend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

    ConstReverseIterator crbegin() const noexcept
    {
        return ConstReverseIterator( end() );
    }

    ConstReverseIterator crend() const noexcept
    {
        return ConstReverseIterator( begin() );
    }

private:
    ElementType* View;
    SizeType ViewSize;
};
