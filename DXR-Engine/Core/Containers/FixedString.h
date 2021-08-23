#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"
#include "Core/Templates/IsTStringType.h"

#include <cstring>
#include <cwchar>

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
        Format(Format.RawString(), List);
    }

    /* Returns this string in lowercase */
    FORCEINLINE void ToLowerInline()
    {
    }

    /* Returns this string in lowercase */
    FORCEINLINE TFixedString ToLower() const 
    {
    }

    /* Converts this string in uppercase */
    FORCEINLINE void ToUpperInline()
    {
    }

    /* Returns this string in uppercase */
    FORCEINLINE TFixedString ToUpper() const 
    {
    }

    /* Return a null terminated string */
    FORCEINLINE CharType* RawString()
    {
        return Elements;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* RawString() const
    {
        return Elements;
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

private:
    CharType Elements[NumChars];
    SizeType Length;
};

/* Predefined types */
template<uint32 NumChars>
using FixedString = TFixedString<char, NumChars>;

template<uint32 NumChars>
using WFixedString = TFixedString<wchar_t, NumChars>;

// Add TFixedString to be a string-type
template<typename CharType, int32 NumChars>
struct TIsTStringType<TFixedString<CharType, NumChars>>
{
    enum
    {
        Value = true;
    };
};