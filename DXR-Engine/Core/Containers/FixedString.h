#pragma once
#include "StringView.h"

/* String class with a fixed allocated number of characters */
template<typename CharType, uint32 NumChars>
class TFixedString
{
public:
    using ElementType = CharType;
    using SizeType    = int32;

    /* Iterators */
    typedef TArrayIterator<TFixedString, CharType>                    IteratorType;
    typedef TArrayIterator<const TFixedString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TFixedString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TFixedString, const CharType> ReverseConstIteratorType;

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");
    static_assert(NumChars > 0, "The number of chars has to be more than zero");

    /* Empty constructor */
    FORCEINLINE TFixedString()
        : Elements()
        , Length(0)
    {
        Memory::Memzero( Elements, sizeof(CharType) * NumChars );
    }

    /* Create a fixed string from a raw array. If the string is longer than 
       the allocators the string will be shortened to fit. */ 
    FORCEINLINE TFixedString( const CharType* InString )
        : Elements()
        , Length( NMath::Min( StrLen( InString ), NumChars) )
    {
        if (InString)
        {
            Memory::Memcpy( Elements, InString, SizeInBytes() );
        }
    }

    /* Create a fixed string from a raw array and the length. If the string is 
       longer than the allocators the string will be shortened to fit. */ 
    FORCEINLINE explicit TFixedString( const CharType* InString, uint32 InLength )
        : Elements()
        , Length( NMath::Min(InLength, NumChars) )
    {
        if (InString)
        {
            Memory::Memcpy( Elements, InString, SizeInBytes() );
        }
    }

    /* Create a string from a bounded array */
    template<const SizeType N>
    FORCEINLINE explicit TFixedString( CharType( &InString )[N] ) noexcept
        : Elements( InArray )
        , Length( NMath::Min(N, NumChars) )
    {
        Memory::Memcpy( Elements, InString, SizeInBytes() );
    }

    /* Create a new string from another string type with similar interface. If the
       string is longer than the allocators the string will be shortened to fit. */ 
    template<typename StringType>
    FORCEINLINE explicit TFixedString( const StringType& InString )
        : Elements()
        , Length( NMath::Min(InString.Length(), NumChars) )
    {
        Memory::Memcpy( Elements, InString.RawString(), SizeInBytes() );
    }

    /* Format string (similar to snprintf) */
    FORCEINLINE void Format(const CharType* Format, ...)
    {
        va_list List;
        va_start(List, Format);
        Format(Format, List);
        va_end(List);
    }

    /* Format string (similar to snprintf) */
    template<typename StringType>
    FORCEINLINE void Format(const StringType& Format, ...)
    {
        va_list List;
        va_start(List, Format);
        Format(Format.RawString(), List);
        va_end(List);
    }

    /* Format string with a va_list (similar to snprintf) */
    FORCEINLINE void Format(const CharType* Format, va_list Args)
    {
        VSNPrintF(ElementType, NumChars, Format, Args);
    }

    /* Format string with a va_list (similar to snprintf) */    
    template<typename StringType>
    FORCEINLINE void Format(const StringType& Format, va_list Args
    {
        Format(Format.CStr(), List);
    }

    /* Returns this string in lowercase */
    FORCEINLINE void ToLowerInline()
    {
        for (SizeType Index = 0; Index < NumChars; Index++)
        {
            Elements[Index] = ToLowerSingle(Elements[Index]);
        }
    }

    /* Returns this string in lowercase */
    FORCEINLINE TFixedString ToLower() const 
    {
        TFixedString Result = *this;
        Result.ToLowerInline();
        return Result;
    }

    /* Converts this string in uppercase */
    FORCEINLINE void ToUpperInline()
    {
        for (SizeType Index = 0; Index < NumChars; Index++)
        {
            Elements[Index] = ToUpperSingle(Elements[Index]);
        }
    }

    /* Returns this string in uppercase */
    FORCEINLINE TFixedString ToUpper() const 
    {
        TFixedString Result = *this;
        Result.ToUpperInline();
        return Result;
    }

    /* Return a null terminated string */
    FORCEINLINE CharType* Data()
    {
        return Elements;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* Data() const
    {
        return Elements;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const
    {
        return Elements;
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Size() const
    {
        return Length;
    }

    /* Returns a sub-string of this string */
    FORCEINLINE TFixedString SubString( SizeType Offset, SizeType Count ) const
    {
        Assert((Offset < Length) && (Offset + Count < Length));
        return TFixedString(Elements + Offset, Count);
    }

    /* Returns a sub-stringview of this string */
    FORCEINLINE TStringView<CharType> SubStringView( SizeType Offset, SizeType Count ) const
    {
        Assert((Offset < Length) && (Offset + Count < Length));
        return TStringView<CharType>(Elements + Offset, Count);
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Length() const
    {
        return Length;
    }

    /* Retrive the size of the string in bytes */
    FORCEINLINE SizeType SizeInBytes() const 
    {
        return Length * sizeof(CharType);
    }

    /* Checks if the string is empty */
    FORCEINLINE bool IsEmpty() const
    {
        return (Length == 0);
    }

public:

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
    CharType Elements[NumChars];
    SizeType Length;
};

/* Predefined types */
template<uint32 NumChars>
using FixedString = TFixedString<char, NumChars>;

template<uint32 NumChars>
using WFixedString = TFixedString<wchar_t, NumChars>;