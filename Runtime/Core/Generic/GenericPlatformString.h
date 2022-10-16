#pragma once
#include "Core/Core.h"

#include <cctype>
#include <cwchar>
#include <cwctype>

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

struct FGenericPlatformString
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CHAR

    NODISCARD
    static FORCEINLINE CHAR ToUpper(CHAR Char)
    {
        return static_cast<CHAR>(::toupper(Char));
    }

    NODISCARD
    static FORCEINLINE CHAR ToLower(CHAR Char)
    {
        return static_cast<CHAR>(::tolower(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsSpace(CHAR Char)
    {
        return static_cast<bool>(::isspace(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsUpper(CHAR Char)
    {
        return static_cast<bool>(::isupper(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsLower(CHAR Char)
    {
        return static_cast<bool>(::islower(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsAlnum(CHAR Char)
    {
        return static_cast<bool>(::isalnum(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsDigit(CHAR Char)
    {
        return static_cast<bool>(::isdigit(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsAlpha(CHAR Char)
    {
        return static_cast<bool>(::isalpha(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsPunct(CHAR Char)
    {
        return static_cast<bool>(::ispunct(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsHexDigit(CHAR Char)
    {
        return static_cast<bool>(::isxdigit(Char));
    }

    template<typename... ArgTypes>
    static FORCEINLINE int32 Sprintf(CHAR* Buffer, const CHAR* Format, ArgTypes&&... Args) noexcept
    {
        return static_cast<int32>(::sprintf(Buffer, Format, Forward<ArgTypes>(Args)...));
    }

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(CHAR* Buffer, TSIZE BufferSize, const CHAR* Format, ArgTypes&&... Args) noexcept
    {
        return static_cast<int32>(::snprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...));
    }

    NODISCARD
    static FORCEINLINE const CHAR* Strstr(const CHAR* String, const CHAR* Find) noexcept
    {
        return static_cast<const CHAR*>(::strstr(String, Find));
    }

    NODISCARD
    static FORCEINLINE const CHAR* Strpbrk(const CHAR* String, const CHAR* Find) noexcept
    {
        return static_cast<const CHAR*>(::strpbrk(String, Find));
    }

    NODISCARD
    static FORCEINLINE const CHAR* Strchr(const CHAR* String, CHAR Char) noexcept
    {
        return static_cast<const CHAR*>(::strchr(String, Char));
    }

    NODISCARD
    static FORCEINLINE const CHAR* Strrchr(const CHAR* String, CHAR Char) noexcept
    {
        return static_cast<const CHAR*>(::strrchr(String, Char));
    }
    
    NODISCARD
    static FORCEINLINE int32 Strlen(const CHAR* String) noexcept
    {
        return static_cast<int32>(::strlen(String));
    }

    NODISCARD
    static FORCEINLINE TSIZE Strspn(const CHAR* String, const CHAR* Set) noexcept
    {
        return static_cast<TSIZE>(::strspn(String, Set));
    }

    static FORCEINLINE CHAR* Strcpy(CHAR* Dst, const CHAR* Src) noexcept
    {
        return static_cast<CHAR*>(::strcpy(Dst, Src));
    }

    static FORCEINLINE CHAR* Strncpy(CHAR* Dst, const CHAR* Src, TSIZE InLength) noexcept
    {
        return static_cast<CHAR*>(::strncpy(Dst, Src, InLength));
    }

    NODISCARD
    static FORCEINLINE int32 Strcmp(const CHAR* String0, const CHAR* String1) noexcept
    {
        return static_cast<int32>(::strcmp(String0, String1));
    }

    NODISCARD
    static FORCEINLINE int32 Strncmp(const CHAR* String0, const CHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::strncmp(String0, String1, InLength));
    }

    NODISCARD
    static FORCEINLINE int32 Strtoi(const CHAR* String, CHAR** End, int32 Base) noexcept
    {
        return static_cast<int32>(::strtol(String, End, Base));
    }

    NODISCARD
    static FORCEINLINE int64 Strtoi64(const CHAR* String, CHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::strtoll(String, End, Base));
    }

    NODISCARD
    static FORCEINLINE uint32 Strtoui(const CHAR* String, CHAR** End, int32 Base) noexcept
    {
        return static_cast<uint32>(::strtoul(String, End, Base));
    }

    NODISCARD
    static FORCEINLINE uint64 Strtoui64(const CHAR* String, CHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::strtoll(String, End, Base));
    }

    NODISCARD
    static FORCEINLINE float Strtof(const CHAR* String, CHAR** End) noexcept
    {
        return static_cast<float>(::strtof(String, End));
    }

    NODISCARD
    static FORCEINLINE double Strtod(const CHAR* String, CHAR** End) noexcept
    {
        return static_cast<float>(::strtod(String, End));
    }

    NODISCARD
    static FORCEINLINE long double Strtold(const CHAR* String, CHAR** End) noexcept
    {
        return static_cast<long double>(::strtold(String, End));
    }
    
    NODISCARD
    static FORCEINLINE int32 Atoi(const CHAR* String) noexcept
    {
        return static_cast<int32>(::atol(String));
    }

    NODISCARD
    static FORCEINLINE int64 Atoi64(const CHAR* String) noexcept
    {
        return static_cast<int64>(::atoll(String));
    }

    NODISCARD
    static FORCEINLINE float Atof(const CHAR* String) noexcept
    {
        return static_cast<float>(::atof(String));
    }

    NODISCARD
    static FORCEINLINE double Atod(const CHAR* String) noexcept
    {
        return static_cast<double>(::atof(String));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // WIDECHAR

    NODISCARD
    static FORCEINLINE WIDECHAR ToUpper(WIDECHAR Char)
    {
        return static_cast<WIDECHAR>(::towupper(Char));
    }

    NODISCARD
    static FORCEINLINE WIDECHAR ToLower(WIDECHAR Char)
    {
        return static_cast<WIDECHAR>(::towlower(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsSpace(WIDECHAR Char)
    {
        return static_cast<bool>(::iswspace(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsUpper(WIDECHAR Char)
    {
        return static_cast<bool>(::iswupper(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsLower(WIDECHAR Char)
    {
        return static_cast<bool>(::iswlower(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsAlnum(WIDECHAR Char)
    {
        return static_cast<bool>(::iswalnum(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsDigit(WIDECHAR Char)
    {
        return static_cast<bool>(::iswdigit(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsAlpha(WIDECHAR Char)
    {
        return static_cast<bool>(::iswalpha(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsPunct(WIDECHAR Char)
    {
        return static_cast<bool>(::iswpunct(Char));
    }

    NODISCARD
    static FORCEINLINE bool IsHexDigit(WIDECHAR Char)
    {
        return static_cast<bool>(::iswxdigit(Char));
    }

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(WIDECHAR* Buffer, TSIZE BufferSize, const WIDECHAR* Format, ArgTypes&&... Args) noexcept
    {
        return static_cast<int32>(::swprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...));
    }

    NODISCARD
    static FORCEINLINE const WIDECHAR* Strstr(const WIDECHAR* String, const WIDECHAR* Find) noexcept
    {
        return static_cast<const WIDECHAR*>(::wcsstr(String, Find));
    }

    NODISCARD
    static FORCEINLINE const WIDECHAR* Strpbrk(const WIDECHAR* String, const WIDECHAR* Find) noexcept
    {
        return static_cast<const WIDECHAR*>(::wcspbrk(String, Find));
    }

    NODISCARD
    static FORCEINLINE const WIDECHAR* Strchr(const WIDECHAR* String, WIDECHAR Char) noexcept
    {
        return static_cast<const WIDECHAR*>(::wcschr(String, Char));
    }

    NODISCARD
    static FORCEINLINE const WIDECHAR* Strrchr(const WIDECHAR* String, WIDECHAR Char) noexcept
    {
        return static_cast<const WIDECHAR*>(::wcsrchr(String, Char));
    }

    NODISCARD
    static FORCEINLINE int32 Strlen(const WIDECHAR* String) noexcept
    {
        return static_cast<int32>(::wcslen(String));
    }

    NODISCARD
    static FORCEINLINE int32 Strspn(const WIDECHAR* String, const WIDECHAR* Set) noexcept
    {
        return static_cast<int32>(::wcsspn(String, Set));
    }

    static FORCEINLINE WIDECHAR* Strcpy(WIDECHAR* Dst, const WIDECHAR* Src) noexcept
    {
        return static_cast<WIDECHAR*>(::wcscpy(Dst, Src));
    }

    static FORCEINLINE WIDECHAR* Strncpy(WIDECHAR* Dst, const WIDECHAR* Src, TSIZE InLength) noexcept
    {
        return static_cast<WIDECHAR*>(::wcsncpy(Dst, Src, InLength));
    }

    NODISCARD
    static FORCEINLINE int32 Strcmp(const WIDECHAR* String0, const WIDECHAR* String1) noexcept
    {
        return static_cast<int32>(::wcscmp(String0, String1));
    }

    NODISCARD
    static FORCEINLINE int32 Strncmp(const WIDECHAR* String0, const WIDECHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::wcsncmp(String0, String1, InLength));
    }

    NODISCARD
    static FORCEINLINE int32 Strtoi(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<int32>(::wcstol(String, End, Base));
    }
    
    NODISCARD
    static FORCEINLINE int64 Strtoi64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::wcstoll(String, End, Base));
    }
    
    NODISCARD
    static FORCEINLINE uint32 Strtoui(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<uint32>(::wcstoul(String, End, Base));
    }
    
    NODISCARD
    static FORCEINLINE uint64 Strtoui64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::wcstoll(String, End, Base));
    }
    
    NODISCARD
    static FORCEINLINE float Strtof(const WIDECHAR* String, WIDECHAR** End) noexcept
    {
        return static_cast<float>(::wcstof(String, End));
    }

    NODISCARD
    static FORCEINLINE double Strtod(const WIDECHAR* String, WIDECHAR** End) noexcept
    {
        return static_cast<double>(::wcstod(String, End));
    }

    NODISCARD
    static FORCEINLINE long double Strtold(const WIDECHAR* String, WIDECHAR** End) noexcept
    {
        return static_cast<long double>(::wcstold(String, End));
    }

    NODISCARD
    static FORCEINLINE int32 Atoi(const WIDECHAR* String) noexcept
    {
        return static_cast<int32>(::wcstol(String, nullptr, 10));
    }

    NODISCARD
    static FORCEINLINE int64 Atoi64(const WIDECHAR* String) noexcept
    {
        return static_cast<int64>(::wcstoll(String, nullptr, 10));
    }

    NODISCARD
    static FORCEINLINE float Atof(const WIDECHAR* String) noexcept
    {
        return static_cast<float>(::wcstof(String, nullptr));
    }

    NODISCARD
    static FORCEINLINE double Atod(const WIDECHAR* String) noexcept
    {
        return static_cast<double>(::wcstod(String, nullptr));
    }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
