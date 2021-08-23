#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "Iterator.h"

#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Move.h"

#include <cstring>
#include <cwchar>
#include <cctype>
#include <cwctype>

/* Wrapper for strlen */
inline int32 StrLen( const char* Str )
{
    return (Str != nullptr) ? static_cast<int32>(strlen( Str )) : 0;
}

/* Wrapper for strlen */
inline int32 StrLen( const wchar_t* Str )
{
    return (Str != nullptr) ? static_cast<int32>(wcslen( Str )) : 0;
}

/* Wrapper for vsnprintf */
inline int32 VSNPrintF( char* Data, int32 Len, const char* Format, va_list Args )
{
    return vsnprintf( Data, Len, Format, Args );
}

/* Wrapper for vsnprintf */
inline int32 VSNPrintF( wchar_t* Data, int32 Len, const wchar_t* Format, va_list Args )
{
    return vswprintf( Data, Len, Format, Args );
}

/* Wrapper for ToLower */
inline char ToLowerSingle( char Char )
{
    return static_cast<char>(tolower(static_cast<int>(Char)));
}

/* Wrapper for ToLower */
inline wchar_t ToLowerSingle( wchar_t Char )
{
    return static_cast<wchar_t>(towlower(static_cast<wint_t>(Char)));
}

/* Wrapper for ToUpper */
inline char ToUpperSingle( char Char )
{
    return static_cast<char>(toupper(static_cast<int>(Char)));
}

/* Wrapper for ToUpper */
inline wchar_t ToUpperSingle( wchar_t Char )
{
    return static_cast<wchar_t>(towupper(static_cast<wint_t>(Char)));
}

/* Class containing a view of a string */
template<typename CharType>
class TStringView
{
public:

    using ElementType = CharType;
    using SizeType    = int32;

    enum
    {
        InvalidPosition = SizeType(-1)
    }

    /* Iterators */
    typedef TArrayIterator<const TFixedString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<const TFixedString, const CharType> ReverseConstIteratorType;

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");

    /* Default construct an empty view */
    FORCEINLINE TStringView() noexcept
        : String( nullptr )
        , Length( 0 )
    {
    }

    /* Create a view from a templated string type */
    template<typename StringType>
    FORCEINLINE explicit TStringView( const StringType& InString ) noexcept
        : String( InArray.CStr() )
        , Length( InArray.Length() )
    {
    }

    /* Create a view from a bounded array */
    template<SizeType N>
    FORCEINLINE explicit TStringView( CharType( &InString )[N] ) noexcept
        : String( InString )
        , Length( N )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE explicit TStringView( const CharType* InString, SizeType Count ) noexcept
        : String( InString )
        , Length( Count )
    {
    }

    /* Create a view from another view */
    FORCEINLINE TStringView( const TStringView& Other ) noexcept
        : String( Other.String )
        , Length( Other.Length )
    {
    }

    /* Move anther view into this one */
    FORCEINLINE TStringView( TStringView&& Other ) noexcept
        : String( Other.String )
        , Length( Other.Length )
    {
        Other.String = nullptr;
        Other.Length = 0;
    }

    /* Check if the size is zero or not */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Length == 0);
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
        return Data()[Length - 1];
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const CharType& At( SizeType Index ) const noexcept
    {
        Assert( Index < Length );
        return Data()[Index];
    }

    /* Swap two views */
    FORCEINLINE void Swap( TStringView& Other ) noexcept
    {
        Swap<const CharType*>( String, Other.String );
        Swap<SizeType>( Length, Other.Length );
    }

    /* Retrive the last valid index for the view */
    FORCEINLINE SizeType LastIndex() const noexcept
    {
        return Length > 0 ? Length - 1 : 0;
    }

    /* Retrive the size of the view */
    FORCEINLINE SizeType Size() const noexcept
    {
        return Length;
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
        Assert((Count < Length) && (Offset + Count < Length) );
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
    SizeType Length = 0;
};