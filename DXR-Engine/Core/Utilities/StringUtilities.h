#pragma once
#if 0

#include <string>
#include <codecvt>
#include <locale>

#include "Core/Containers/String.h"

inline WString ConvertToWide( const String& AsciiString )
{
    WString WideString = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes( AsciiString.c_str() );
    return WideString;
}

inline String ConvertToAscii( const WString& WideString )
{
    String AsciiString = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes( WideString.c_str() );
    return AsciiString;
}

inline void ConvertBackslashes( String& OutString )
{
    auto pos = OutString.find_first_of( '\\' );
    while ( pos != String::npos )
    {
        OutString.replace( pos, 1, 1, '/' );
        pos = OutString.find_first_of( '\\', pos + 1 );
    }
}

inline String& ToLower( String& InString )
{
    for ( char& Char : InString )
    {
        Char = (char)tolower( Char );
    }

    return InString;
}

inline String ToLower( const String& InString )
{
    String Result = InString;
    for ( char& Char : Result )
    {
        Char = (char)tolower( Char );
    }

    return Result;
}

inline WString& ToLower( WString& InString )
{
    for ( wchar_t& Char : InString )
    {
        Char = (wchar_t)towlower( Char );
    }

    return InString;
}

inline WString ToLower( const WString& InString )
{
    WString Result = InString;
    for ( wchar_t& Char : Result )
    {
        Char = (wchar_t)towlower( Char );
    }

    return Result;
}

#endif