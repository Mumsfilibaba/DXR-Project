#pragma once
#include <string>
#include <codecvt>
#include <locale>

inline std::wstring ConvertToWide(const std::string& AsciiString)
{
    std::wstring WideString = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(AsciiString.c_str());
    return WideString;
}

inline std::string ConvertToAscii(const std::wstring& WideString)
{
    std::string AsciiString = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(WideString.c_str());
    return AsciiString;
}

inline void ConvertBackslashes(std::string& OutString)
{
    size_t Pos = OutString.find_first_of('\\');
    while (Pos != std::string::npos)
    {
        OutString.replace(Pos, 1, 1, '/');
        Pos = OutString.find_first_of('\\', Pos + 1);
    }
}