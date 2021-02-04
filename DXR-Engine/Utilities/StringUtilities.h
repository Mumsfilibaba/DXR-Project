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
    size_t pos = OutString.find_first_of('\\');
    while (pos != std::string::npos)
    {
        OutString.replace(pos, 1, 1, '/');
        pos = OutString.find_first_of('\\', pos + 1);
    }
}