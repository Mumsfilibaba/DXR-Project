#pragma once
#include "CoreDefines.h"
#include "CoreTypes.h"

#include <cstring>
#include <cwchar>
#include <cctype>
#include <cwctype>
#include <cstdarg>

/* Helpers for characters */
template<typename CharType>
class TStringTraits;

template<>
class TStringTraits<char>
{
public:

    using CharType = char;

    enum
    {
        Terminator = '\0'
    };

    /* Is space */
    static FORCEINLINE bool IsSpace( CharType Char ) noexcept
    {
        return !!isspace( static_cast<int>(Char) );
    }

    /* Is Nullterminator */
    static FORCEINLINE bool IsTerminator( CharType Char ) noexcept
    {
        return (Char == Terminator);
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
    static FORCEINLINE int32 PrintVA( CharType* Data, int32 Len, const CharType* Format, va_list Args ) noexcept
    {
        return vsnprintf( Data, Len, Format, Args );
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
    static FORCEINLINE CharType* Copy( CharType* Destination, const CharType* Source, uint64 Length ) noexcept
    {
        return reinterpret_cast<CharType*>(Memory::Memcpy( Destination, Source, Length * sizeof( CharType ) ));
    }

    /* Compare two strings */
    static FORCEINLINE int32 Compare( const CharType* LHS, const CharType* RHS ) noexcept
    {
        return static_cast<int32>(strcmp( LHS, RHS ));
    }
};

template<>
class TStringTraits<wchar_t>
{
public:

    using CharType = wchar_t;

    enum
    {
        Terminator = L'\0'
    };

    /* Is space */
    static FORCEINLINE bool IsSpace( CharType Char ) noexcept
    {
        return !!iswspace( static_cast<wint_t>(Char) );
    }

    /* Is Nullterminator */
    static FORCEINLINE bool IsTerminator( CharType Char ) noexcept
    {
        return (Char == Terminator);
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
    static FORCEINLINE int32 PrintVA( CharType* Data, int32 Len, const CharType* Format, va_list Args ) noexcept
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
    static FORCEINLINE CharType* Copy( CharType* Destination, const CharType* Source, uint64 Length ) noexcept
    {
        return reinterpret_cast<CharType*>(Memory::Memcpy( Destination, Source, Length * sizeof( CharType ) ));
    }

    /* Compare two strings */
    static FORCEINLINE int32 Compare( const CharType* LHS, const CharType* RHS ) noexcept
    {
        return static_cast<int32>(wcscmp( LHS, RHS ));
    }
};

/* Predefined types */
using CStringTraits = TStringTraits<char>;
using WStringTraits = TStringTraits<wchar_t>;