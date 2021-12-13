#pragma once
#include "Core/Memory/Memory.h"

#include <cstring>
#include <cwchar>
#include <cctype>
#include <cwctype>
#include <cstdarg>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers for characters */
template<typename CharType>
class TStringUtils;

template<>
class TStringUtils<char>
{
public:

    using CharType = char;
    using Pointer = CharType*;
    using ConstPointer = const Pointer;

    static constexpr CharType Null = '\0';

    /* Is space */
    static FORCEINLINE bool IsSpace( CharType Char ) noexcept
    {
        return !!isspace( static_cast<int>(Char) );
    }

    /* Is null-terminator */
    static FORCEINLINE bool IsTerminator( CharType Char ) noexcept
    {
        return (Char == Null);
    }

    /* Is space or null terminator */
    static FORCEINLINE bool IsWhiteSpace( CharType Char ) noexcept
    {
        return IsSpace( Char ) || IsTerminator( Char );
    }

    /* Finds substring */
    static FORCEINLINE const CharType* Find( const CharType* String, const CharType* Substring ) noexcept
    {
        return strstr( String, Substring );
    }

    /* Finds the first occurance of one of the characters in the Set*/
    static FORCEINLINE const CharType* FindOneOf( const CharType* String, const CharType* Set ) noexcept
    {
        return strpbrk( String, Set );
    }

    /* Finds the length of the span of characters only from the set */
    static FORCEINLINE int32 Span( const CharType* String, const CharType* Set ) noexcept
    {
        return static_cast<int32>(strspn( String, Set ));
    }

    /* Finds character in string */
    static FORCEINLINE const CharType* FindChar( const CharType* InString, CharType InChar ) noexcept
    {
        return strchr( InString, InChar );
    }

    /* Finds character in string by searching backwards */
    static FORCEINLINE const CharType* ReverseFindChar( const CharType* InString, CharType InChar ) noexcept
    {
        return strrchr( InString, InChar );
    }

    /* Length of string */
    static FORCEINLINE int32 Length( const CharType* Str ) noexcept
    {
        return (Str != nullptr) ? static_cast<int32>(strlen( Str )) : 0;
    }

    /* Format into buffer */
    static int32 FormatBuffer( CharType* Buffer, int32 Len, const CharType* Format, ... ) noexcept
    {
        va_list Args;
        va_start( Args, Format );
        int32 Result = FormatBufferV( Buffer, Len, Format, Args );
        va_end( Args );

        return Result;
    }

    /* Format into buffer */
    static FORCEINLINE int32 FormatBufferV( CharType* Buffer, int32 Len, const CharType* Format, va_list Args ) noexcept
    {
        return vsnprintf( Buffer, Len, Format, Args );
    }

    /* Convert to lower-case */
    static FORCEINLINE CharType ToLower( char Char ) noexcept
    {
        return static_cast<CharType>(tolower( static_cast<int>(Char) ));
    }

    /* Convert to lower-case */
    static FORCEINLINE CharType ToUpper( char Char ) noexcept
    {
        return static_cast<CharType>(toupper( static_cast<int>(Char) ));
    }

    /* Copy two strings */
    static FORCEINLINE CharType* Copy( CharType* Destination, const CharType* Source ) noexcept
    {
        return Copy( Destination, Source, Length( Source ) );
    }

    /* Copy two strings */
    static FORCEINLINE CharType* Copy( CharType* Destination, const CharType* Source, uint64 Length ) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memcpy( Destination, Source, Length * sizeof( CharType ) ));
    }

    /* Move two strings */
    static FORCEINLINE CharType* Move( CharType* Destination, const CharType* Source, uint64 Length ) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memmove( Destination, Source, Length * sizeof( CharType ) ));
    }

    /* Compare two strings */
    static FORCEINLINE int32 Compare( const CharType* LHS, const CharType* RHS ) noexcept
    {
        return static_cast<int32>(strcmp( LHS, RHS ));
    }

    /* Compare two strings */
    static FORCEINLINE int32 Compare( const CharType* LHS, const CharType* RHS, uint64 InLength ) noexcept
    {
        return static_cast<int32>(strncmp( LHS, RHS, InLength ));
    }

    /* Returns the empty string */
    static FORCEINLINE const CharType* Empty() noexcept
    {
        return "";
    }
};

template<>
class TStringUtils<wchar_t>
{
public:

    using CharType = wchar_t;
    using Pointer = CharType*;
    using ConstPointer = const Pointer;

    static constexpr CharType Null = L'\0';

    /* Is space */
    static FORCEINLINE bool IsSpace( CharType Char ) noexcept
    {
        return !!iswspace( static_cast<wint_t>(Char) );
    }

    /* Is null-terminator */
    static FORCEINLINE bool IsTerminator( CharType Char ) noexcept
    {
        return (Char == Null);
    }

    /* Is space or null terminator */
    static FORCEINLINE bool IsWhiteSpace( CharType Char ) noexcept
    {
        return IsSpace( Char ) || IsTerminator( Char );
    }

    /* Finds substring */
    static FORCEINLINE const CharType* Find( const CharType* String, const CharType* Substring ) noexcept
    {
        return wcsstr( String, Substring );
    }

    /* Finds the first occurance of one of the characters */
    static FORCEINLINE const CharType* FindOneOf( const CharType* String, const CharType* Set ) noexcept
    {
        return wcspbrk( String, Set );
    }

    /* Finds character in string */
    static FORCEINLINE const CharType* FindChar( const CharType* String, CharType Char ) noexcept
    {
        return wcschr( String, Char );
    }

    /* Finds character in string by searching backwards */
    static FORCEINLINE const CharType* ReverseFindChar( const CharType* String, CharType Char ) noexcept
    {
        return wcsrchr( String, Char );
    }

    /* Finds the length of the span of characters only from the set */
    static FORCEINLINE int32 Span( const CharType* String, const CharType* Set ) noexcept
    {
        return static_cast<int32>(wcsspn( String, Set ));
    }

    /* Length of string */
    static FORCEINLINE int32 Length( const CharType* Str ) noexcept
    {
        return (Str != nullptr) ? static_cast<int32>(wcslen( Str )) : 0;
    }

    /* Format into buffer */
    static int32 FormatBuffer( CharType* Buffer, int32 Len, const CharType* Format, ... ) noexcept
    {
        va_list Args;
        va_start( Args, Format );
        int32 Result = FormatBufferV( Buffer, Len, Format, Args );
        va_end( Args );

        return Result;
    }

    /* Format into buffer */
    static FORCEINLINE int32 FormatBufferV( CharType* Data, int32 Len, const CharType* Format, va_list Args ) noexcept
    {
        return vswprintf( Data, Len, Format, Args );
    }

    /* Convert to lower-case */
    static FORCEINLINE CharType ToLower( CharType Char ) noexcept
    {
        return static_cast<CharType>(towlower( static_cast<wint_t>(Char) ));
    }

    /* Convert to upper-case */
    static FORCEINLINE CharType ToUpper( CharType Char ) noexcept
    {
        return static_cast<CharType>(towupper( static_cast<wint_t>(Char) ));
    }

    /* Copy two strings */
    static FORCEINLINE CharType* Copy( CharType* Destination, const CharType* Source ) noexcept
    {
        return Copy( Destination, Source, Length( Source) );
    }

    /* Copy two strings */
    static FORCEINLINE CharType* Copy( CharType* Destination, const CharType* Source, uint64 Length ) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memcpy( Destination, Source, Length * sizeof( CharType ) ));
    }

    /* Move two strings */
    static FORCEINLINE CharType* Move( CharType* Destination, const CharType* Source, uint64 Length ) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memmove( Destination, Source, Length * sizeof( CharType ) ));
    }

    /* Compare two strings */
    static FORCEINLINE int32 Compare( const CharType* LHS, const CharType* RHS ) noexcept
    {
        return static_cast<int32>(wcscmp( LHS, RHS ));
    }

    /* Compare two strings */
    static FORCEINLINE int32 Compare( const CharType* LHS, const CharType* RHS, uint64 InLength ) noexcept
    {
        return static_cast<int32>(wcsncmp( LHS, RHS, InLength ));
    }

    /* Returns the empty string */
    static FORCEINLINE const CharType* Empty() noexcept
    {
        return L"";
    }
};

/* Predefined types */
using CStringUtils = TStringUtils<char>;
using WStringUtils = TStringUtils<wchar_t>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers for StringParsing */

template<typename CharType>
class TStringParse;

template<>
class TStringParse<char>
{
public:

    using CharType = char;

    static FORCEINLINE int8 ParseInt8( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int8>(strtol( String, End, Base ));
    }

    static FORCEINLINE int16 ParseInt16( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int16>(strtol( String, End, Base ));
    }

    static FORCEINLINE int32 ParseInt32( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int32>(strtol( String, End, Base ));
    }

    static FORCEINLINE int64 ParseInt64( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int64>(strtoll( String, End, Base ));
    }

    static FORCEINLINE int8 ParseUint8( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int8>(strtoul( String, End, Base ));
    }

    static FORCEINLINE int16 ParseUint16( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int16>(strtoul( String, End, Base ));
    }

    static FORCEINLINE int32 ParseUint32( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int32>(strtoul( String, End, Base ));
    }

    static FORCEINLINE int64 ParseUint64( const CharType* String, CharType** End, int32 Base )
    {
        return static_cast<int64>(strtoull( String, End, Base ));
    }

    static FORCEINLINE float ParseFloat( const CharType* String, CharType** End )
    {
        return static_cast<float>(strtold( String, End ));
    }

    static FORCEINLINE double ParseDouble( const CharType* String, CharType** End )
    {
        return static_cast<double>(strtold( String, End ));
    }

    template<typename T>
    static T ParseInt( const CharType* String, CharType** End, int32 Base );

    template<>
    static FORCEINLINE int8 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseInt8( String, End, Base ); }

    template<>
    static FORCEINLINE int16 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseInt16( String, End, Base ); }

    template<>
    static FORCEINLINE int32 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseInt32( String, End, Base ); }

    template<>
    static FORCEINLINE int64 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseInt64( String, End, Base ); }

    template<>
    static FORCEINLINE uint8 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseUint8( String, End, Base ); }

    template<>
    static FORCEINLINE uint16 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseUint16( String, End, Base ); }

    template<>
    static FORCEINLINE uint32 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseUint32( String, End, Base ); }

    template<>
    static FORCEINLINE uint64 ParseInt( const CharType* String, CharType** End, int32 Base ) { return ParseUint64( String, End, Base ); }
};

/* Predefined types */
using CStringParse = TStringParse<char>;
using WStringParse = TStringParse<wchar_t>;