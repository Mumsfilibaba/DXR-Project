#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "Iterator.h"

#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/IsTStringType.h"
#include "Core/Templates/StringTraits.h"

/* Class containing a view of a string */
template<typename CharType>
class TStringView
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");
    
    /* Types */
    using ElementType  = CharType;
    using SizeType     = int32;
    using StringTraits = TStringTraits<CharType>;

    /* Constants */
    enum
    {
        InvalidPosition = SizeType( -1 )
    };

    /* Iterators */
    typedef TArrayIterator<const TStringView, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<const TStringView, const CharType> ReverseConstIteratorType;

    /* Default construct an empty view */
    FORCEINLINE TStringView() noexcept
        : String( nullptr )
        , ViewLength( 0 )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE TStringView( const CharType* InString ) noexcept
        : String( InString )
        , ViewLength( StringTraits::Length( InString ) )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE explicit TStringView( const CharType* InString, SizeType Count ) noexcept
        : String( InString )
        , ViewLength( Count )
    {
    }

    /* Create a view from a templated string type */
    template<typename StringType>
    FORCEINLINE explicit TStringView( const StringType& InString ) noexcept
        : String( InString.CStr() )
        , ViewLength( InString.Length() )
    {
    }

    /* Create a view from another view */
    FORCEINLINE TStringView( const TStringView& Other ) noexcept
        : String( Other.String )
        , ViewLength( Other.ViewLength )
    {
    }

    /* Move anther view into this one */
    FORCEINLINE TStringView( TStringView&& Other ) noexcept
        : String( Other.String )
        , ViewLength( Other.ViewLength )
    {
        Other.String = nullptr;
        Other.ViewLength = 0;
    }

    /* Check if the size is zero or not */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ViewLength == 0);
    }

    /* Retrive the first element */
    FORCEINLINE const CharType& FirstElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[0];
    }

    /* Retrive the last element */
    FORCEINLINE const CharType& LastElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[ViewLength - 1];
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const CharType& At( SizeType Index ) const noexcept
    {
        Assert( Index < ViewLength );
        return Data()[Index];
    }

    /* Swap two views */
    FORCEINLINE void Swap( TStringView& Other ) noexcept
    {
        Swap<const CharType*>( String, Other.String );
        Swap<SizeType>( ViewLength, Other.ViewLength );
    }

    /* Retrive the last valid index for the view */
    FORCEINLINE SizeType LastIndex() const noexcept
    {
        return ViewLength > 0 ? ViewLength - 1 : 0;
    }

    /* Retrive the size of the view */
    FORCEINLINE SizeType Size() const noexcept
    {
        return ViewLength;
    }

    /* Retrive the length of the view */
    FORCEINLINE SizeType Length() const noexcept
    {
        return ViewLength;
    }

    /* Retrive the size of the view in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof( CharType );
    }

    /* Retrive the data of the view */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return String;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return String;
    }

    /* Create a sub-stringview */
    FORCEINLINE TStringView SubStringView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert((Count < ViewLength) && (Offset + Count < ViewLength) );
        return TStringView( Data() + Offset, Count );
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename StringType>
    FORCEINLINE bool operator==( const StringType& Other ) const noexcept
    {
        if ( Length() != Other.Length() )
        {
            return false;
        }

        return CompareRange<CharType>( CStr(), Other.CStr(), Length() );
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename StringType>
    FORCEINLINE bool operator!=( const StringType& Other ) const noexcept
    {
        return !(*this == Other);
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const CharType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    /* Assign from another view */
    FORCEINLINE TStringView& operator=( const TStringView& Other ) noexcept
    {
        TStringView( Other ).Swap( *this );
        return *this;
    }

    /* Move-assign from another view */
    FORCEINLINE TStringView& operator=( TStringView&& Other ) noexcept
    {
        TStringView( Move( Other ) ).Swap( *this );
        return *this;
    }

public:

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
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, 0 );
    }

public:

    /* STL iterator functions - Enables Range-based for-loops */
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:
    const CharType* String = nullptr;
    SizeType ViewLength = 0;
};

/* Predefined types */
using CStringView = TStringView<char>;
using WStringView = TStringView<wchar_t>;