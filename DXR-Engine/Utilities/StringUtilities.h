#pragma once
#include <string>
#include <codecvt>
#include <locale>

using String = std::string;
using WString = std::wstring;

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
    size_t pos = OutString.find_first_of( '\\' );
    while ( pos != String::npos )
    {
        OutString.replace( pos, 1, 1, '/' );
        pos = OutString.find_first_of( '\\', pos + 1 );
    }
}

inline String& ToLower( String& InString )
{
    for ( char& c : InString )
    {
        c = (char)tolower( c );
    }

    return InString;
}

inline String ToLower( const String& InString )
{
    String Result = InString;
    for ( char& c : Result )
    {
        c = (char)tolower( c );
    }

    return Result;
}

inline WString& ToLower( WString& InString )
{
    for ( wchar_t& c : InString )
    {
        c = (wchar_t)towlower( c );
    }

    return InString;
}

inline WString ToLower( const WString& InString )
{
    WString Result = InString;
    for ( wchar_t& c : Result )
    {
        c = (wchar_t)towlower( c );
    }

    return Result;
}