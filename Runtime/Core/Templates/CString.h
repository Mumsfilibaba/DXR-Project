#pragma once
#include "AddPointer.h"

#include "Core/Memory/Memory.h"

#include <cstring>
#include <cwchar>
#include <cctype>
#include <cwctype>
#include <cstdarg>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TChar

template<typename InCharType>
struct TChar 
{
    using CharType         = InCharType;
    using PointerType      = typename TAddPointer<CharType>::Type;
    using ConstPointerType = const PointerType;

    static CONSTEXPR InCharType Terminator = 0;

    static NODISCARD FORCEINLINE CharType ToUpper(CharType Char);
    static NODISCARD FORCEINLINE CharType ToLower(CharType Char);

    static NODISCARD FORCEINLINE bool IsSpace(CharType Char);
    static NODISCARD FORCEINLINE bool IsUpper(CharType Char);
    static NODISCARD FORCEINLINE bool IsLower(CharType Char);
    static NODISCARD FORCEINLINE bool IsAlnum(CharType Char);
    static NODISCARD FORCEINLINE bool IsDigit(CharType Char);
    static NODISCARD FORCEINLINE bool IsAlpha(CharType Char);
    static NODISCARD FORCEINLINE bool IsPunct(CharType Char);
    static NODISCARD FORCEINLINE bool IsHexDigit(CharType Char);

    static NODISCARD FORCEINLINE bool IsTerminator(CharType Char) { return (Char == Terminator); }
};

typedef TChar<CHAR>     FChar;
typedef TChar<WIDECHAR> FCharWide;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FChar

template<>
NODISCARD FORCEINLINE CHAR TChar<CHAR>::ToUpper(CHAR Char) { return static_cast<CHAR>(toupper(Char)); }

template<>
NODISCARD FORCEINLINE CHAR TChar<CHAR>::ToLower(CHAR Char) { return static_cast<CHAR>(tolower(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsSpace(CHAR Char) { return static_cast<bool>(isspace(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsUpper(CHAR Char) { return static_cast<bool>(isupper(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsLower(CHAR Char) { return static_cast<bool>(islower(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsAlnum(CHAR Char) { return static_cast<bool>(isalnum(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsDigit(CHAR Char) { return static_cast<bool>(isdigit(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsAlpha(CHAR Char) { return static_cast<bool>(isalpha(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsPunct(CHAR Char) { return static_cast<bool>(ispunct(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<CHAR>::IsHexDigit(CHAR Char) { return static_cast<bool>(isxdigit(Char)); }

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCharWide

template<>
NODISCARD FORCEINLINE WIDECHAR TChar<WIDECHAR>::ToUpper(WIDECHAR Char) { return static_cast<WIDECHAR>(towupper(Char)); }

template<>
NODISCARD FORCEINLINE WIDECHAR TChar<WIDECHAR>::ToLower(WIDECHAR Char) { return static_cast<WIDECHAR>(towlower(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsSpace(WIDECHAR Char) { return static_cast<bool>(iswspace(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsUpper(WIDECHAR Char) { return static_cast<bool>(iswupper(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsLower(WIDECHAR Char) { return static_cast<bool>(iswlower(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsAlnum(WIDECHAR Char) { return static_cast<bool>(iswalnum(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsDigit(WIDECHAR Char) { return static_cast<bool>(iswdigit(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsAlpha(WIDECHAR Char) { return static_cast<bool>(iswalpha(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsPunct(WIDECHAR Char) { return static_cast<bool>(iswpunct(Char)); }

template<>
NODISCARD FORCEINLINE bool TChar<WIDECHAR>::IsHexDigit(WIDECHAR Char) { return static_cast<bool>(iswxdigit(Char)); }

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TCString

template<typename InCharType>
struct TCString
{
    typedef InCharType CHARTYPE;

    template<typename... ArgTypes>
    static FORCEINLINE int32 Sprintf(CHARTYPE* Buffer, const CHARTYPE* Format, ArgTypes&&... Args) noexcept;

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(CHARTYPE* Buffer, TSIZE BufferSize, const CHARTYPE* Format, ArgTypes&&... Args) noexcept;

    static FORCEINLINE const CHARTYPE* Strstr(const CHARTYPE* String, const CHARTYPE* Substring) noexcept;
    static FORCEINLINE const CHARTYPE* Strpbrk(const CHARTYPE* String, const CHARTYPE* Set)      noexcept;
    static FORCEINLINE const CHARTYPE* Strchr(const CHARTYPE* String, CHARTYPE Char)             noexcept;
    static FORCEINLINE const CHARTYPE* Strrchr(const CHARTYPE* String, CHARTYPE Char)            noexcept;

    static FORCEINLINE int32 Strlen(const CHARTYPE* String)                      noexcept;
    static FORCEINLINE int32 Strspn(const CHARTYPE* String, const CHARTYPE* Set) noexcept;

    static FORCEINLINE CHARTYPE* Strcpy(CHARTYPE* Dest, const CHARTYPE* Source)                   noexcept;
    static FORCEINLINE CHARTYPE* Strncpy(CHARTYPE* Dest, const CHARTYPE* Source, TSIZE InLength)  noexcept;
    static FORCEINLINE CHARTYPE* Strmove(CHARTYPE* Dest, const CHARTYPE* Source)                  noexcept;
    static FORCEINLINE CHARTYPE* Strnmove(CHARTYPE* Dest, const CHARTYPE* Source, TSIZE InLength) noexcept;
    
    static FORCEINLINE int32 Strcmp(const CHARTYPE* LHS, const CHARTYPE* RHS)                  noexcept;
    static FORCEINLINE int32 Strncmp(const CHARTYPE* LHS, const CHARTYPE* RHS, TSIZE InLength) noexcept;

    static FORCEINLINE int32  Strtoi(const CHARTYPE* String, CHARTYPE** End, int32 Base)    noexcept;
    static FORCEINLINE int64  Strtoi64(const CHARTYPE* String, CHARTYPE** End, int32 Base)  noexcept;
    static FORCEINLINE uint32 Strtoui(const CHARTYPE* String, CHARTYPE** End, int32 Base)   noexcept;
    static FORCEINLINE uint64 Strtoui64(const CHARTYPE* String, CHARTYPE** End, int32 Base) noexcept;
    static FORCEINLINE float  Strtof(const CHARTYPE* String, CHARTYPE** End)                noexcept;
    static FORCEINLINE double Strtod(const CHARTYPE* String, CHARTYPE** End)                noexcept;
    
	static FORCEINLINE int32  Atoi(const CHARTYPE* String)   noexcept;
    static FORCEINLINE int64  Atoi64(const CHARTYPE* String) noexcept;
	static FORCEINLINE float  Atof(const CHARTYPE* String)   noexcept;
	static FORCEINLINE double Atod(const CHARTYPE* String)   noexcept;

    static FORCEINLINE const CHARTYPE* Empty() noexcept;
};

typedef TCString<CHAR>     FCString;
typedef TCString<WIDECHAR> FCStringWide;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCString

template<>
template<typename... ArgTypes>
FORCEINLINE int32 TCString<CHAR>::Sprintf(CHAR* Buffer, const CHAR* Format, ArgTypes&&... Args) noexcept
{
    return sprintf(Buffer, Format, Forward<ArgTypes>(Args)...);
}

template<>
template<typename... ArgTypes>
FORCEINLINE int32 TCString<CHAR>::Snprintf(CHAR* Buffer, TSIZE BufferSize, const CHAR* Format, ArgTypes&&... Args) noexcept
{
    return snprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...);
}

template<>
FORCEINLINE const CHAR* TCString<CHAR>::Strstr(const CHAR* String, const CHAR* Substring) noexcept
{
    return strstr(String, Substring);
}

template<>
FORCEINLINE const CHAR* TCString<CHAR>::Strpbrk(const CHAR* String, const CHAR* Set) noexcept
{
    return strpbrk(String, Set);
}

template<>
FORCEINLINE const CHAR* TCString<CHAR>::Strchr(const CHAR* String, CHAR Char) noexcept
{
    return strchr(String, Char);
}

template<>
FORCEINLINE const CHAR* TCString<CHAR>::Strrchr(const CHAR* String, CHAR Char) noexcept
{
    return strrchr(String, Char);
}

template<>
FORCEINLINE int32 TCString<CHAR>::Strspn(const CHAR* String, const CHAR* Set) noexcept
{
    return static_cast<int32>(strspn(String, Set));
}

template<>
FORCEINLINE int32 TCString<CHAR>::Strlen(const CHAR* String) noexcept
{
    return (String != nullptr) ? static_cast<int32>(strlen(String)) : 0;
}

template<>
FORCEINLINE CHAR* TCString<CHAR>::Strcpy(CHAR* Dest, const CHAR* Source) noexcept
{
    return Strncpy(Dest, Source, Strlen(Source));
}

template<>
FORCEINLINE CHAR* TCString<CHAR>::Strncpy(CHAR* Dest, const CHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<CHAR*>(FMemory::Memcpy(Dest, Source, InLength * sizeof(CHAR)));
}

template<>
FORCEINLINE CHAR* TCString<CHAR>::Strmove(CHAR* Dest, const CHAR* Source) noexcept
{
    return Strnmove(Dest, Source, Strlen(Source));
}

template<>
FORCEINLINE CHAR* TCString<CHAR>::Strnmove(CHAR* Dest, const CHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<CHAR*>(FMemory::Memmove(Dest, Source, InLength * sizeof(CHAR)));
}

template<>
FORCEINLINE int32 TCString<CHAR>::Strcmp(const CHAR* LHS, const CHAR* RHS) noexcept
{
    return static_cast<int32>(strcmp(LHS, RHS));
}

template<>
FORCEINLINE int32 TCString<CHAR>::Strncmp(const CHAR* LHS, const CHAR* RHS, uint64 InLength) noexcept
{
    return static_cast<int32>(strncmp(LHS, RHS, InLength));
}

template<>
FORCEINLINE int32 TCString<CHAR>::Strtoi(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<int32>(Strtoi(String, End, Base));
}

template<>
FORCEINLINE int64 TCString<CHAR>::Strtoi64(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<int64>(Strtoi64(String, End, Base));
}

template<>
FORCEINLINE uint32 TCString<CHAR>::Strtoui(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<uint32>(Strtoui(String, End, Base));
}

template<>
FORCEINLINE uint64 TCString<CHAR>::Strtoui64(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<uint64>(Strtoui64(String, End, Base));
}

template<>
FORCEINLINE float TCString<CHAR>::Strtof(const CHAR* String, CHAR** End) noexcept
{
    return static_cast<float>(strtof(String, End));
}

template<>
FORCEINLINE double TCString<CHAR>::Strtod(const CHAR* String, CHAR** End) noexcept
{
    return static_cast<double>(strtold(String, End));
}

template<>
FORCEINLINE int32 TCString<CHAR>::Atoi(const CHAR* String) noexcept
{
    return static_cast<int32>(atoi(String));
}

template<>
FORCEINLINE int64 TCString<CHAR>::Atoi64(const CHAR* String) noexcept
{
    return static_cast<int64>(atoll(String));
}

template<>
FORCEINLINE float TCString<CHAR>::Atof(const CHAR* String) noexcept
{
    return static_cast<float>(atof(String));
}

template<>
FORCEINLINE double TCString<CHAR>::Atod(const CHAR* String) noexcept
{
    return static_cast<double>(strtod(String, nullptr));
}

template<>
FORCEINLINE const CHAR* TCString<CHAR>::Empty() noexcept
{
    return "";
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCStringWide

template<>
template<typename... ArgTypes>
FORCEINLINE int32 TCString<WIDECHAR>::Sprintf(WIDECHAR* Buffer, const WIDECHAR* Format, ArgTypes&&... Args) noexcept
{
    return swprintf(Buffer, Format, Forward<ArgTypes>(Args)...);
}

template<>
template<typename... ArgTypes>
FORCEINLINE int32 TCString<WIDECHAR>::Snprintf(WIDECHAR* Buffer, TSIZE BufferSize, const WIDECHAR* Format, ArgTypes&&... Args) noexcept
{
    return swprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...);
}

template<>
FORCEINLINE const WIDECHAR* TCString<WIDECHAR>::Strstr(const WIDECHAR* String, const WIDECHAR* Substring) noexcept
{
    return wcsstr(String, Substring);
}

template<>
FORCEINLINE const WIDECHAR* TCString<WIDECHAR>::Strpbrk(const WIDECHAR* String, const WIDECHAR* Set) noexcept
{
    return wcspbrk(String, Set);
}

template<>
FORCEINLINE const WIDECHAR* TCString<WIDECHAR>::Strchr(const WIDECHAR* String, WIDECHAR Char) noexcept
{
    return wcschr(String, Char);
}

template<>
FORCEINLINE const WIDECHAR* TCString<WIDECHAR>::Strrchr(const WIDECHAR* String, WIDECHAR Char) noexcept
{
    return wcsrchr(String, Char);
}

template<>
FORCEINLINE int32 TCString<WIDECHAR>::Strspn(const WIDECHAR* String, const WIDECHAR* Set) noexcept
{
    return static_cast<int32>(wcsspn(String, Set));
}

template<>
FORCEINLINE int32 TCString<WIDECHAR>::Strlen(const WIDECHAR* String) noexcept
{
    return (String != nullptr) ? static_cast<int32>(wcslen(String)) : 0;
}

template<>
FORCEINLINE WIDECHAR* TCString<WIDECHAR>::Strcpy(WIDECHAR* Dest, const WIDECHAR* Source) noexcept
{
    return Strncpy(Dest, Source, Strlen(Source));
}

template<>
FORCEINLINE WIDECHAR* TCString<WIDECHAR>::Strncpy(WIDECHAR* Dest, const WIDECHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<WIDECHAR*>(FMemory::Memcpy(Dest, Source, InLength * sizeof(WIDECHAR)));
}

template<>
FORCEINLINE WIDECHAR* TCString<WIDECHAR>::Strmove(WIDECHAR* Dest, const WIDECHAR* Source) noexcept
{
    return Strnmove(Dest, Source, Strlen(Source));
}

template<>
FORCEINLINE WIDECHAR* TCString<WIDECHAR>::Strnmove(WIDECHAR* Dest, const WIDECHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<WIDECHAR*>(FMemory::Memmove(Dest, Source, InLength * sizeof(WIDECHAR)));
}

template<>
FORCEINLINE int32 TCString<WIDECHAR>::Strcmp(const WIDECHAR* LHS, const WIDECHAR* RHS) noexcept
{
    return static_cast<int32>(wcscmp(LHS, RHS));
}

template<>
FORCEINLINE int32 TCString<WIDECHAR>::Strncmp(const WIDECHAR* LHS, const WIDECHAR* RHS, uint64 InLength) noexcept
{
    return static_cast<int32>(wcsncmp(LHS, RHS, InLength));
}

template<>
FORCEINLINE int32 TCString<WIDECHAR>::Strtoi(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
{
    return static_cast<int32>(wcstol(String, End, Base));
}

template<>
FORCEINLINE int64 TCString<WIDECHAR>::Strtoi64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
{
    return static_cast<int64>(wcstoll(String, End, Base));
}

template<>
FORCEINLINE uint32 TCString<WIDECHAR>::Strtoui(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
{
    return static_cast<uint32>(wcstoul(String, End, Base));
}

template<>
FORCEINLINE uint64 TCString<WIDECHAR>::Strtoui64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
{
    return static_cast<uint64>(wcstoull(String, End, Base));
}

template<>
FORCEINLINE float TCString<WIDECHAR>::Strtof(const WIDECHAR* String, WIDECHAR** End) noexcept
{
    return static_cast<float>(wcstof(String, End));
}

template<>
FORCEINLINE double TCString<WIDECHAR>::Strtod(const WIDECHAR* String, WIDECHAR** End) noexcept
{
    return static_cast<double>(wcstod(String, End));
}

template<>
FORCEINLINE int32 TCString<WIDECHAR>::Atoi(const WIDECHAR* String) noexcept
{
    return Strtoi(String, nullptr, 10);
}

template<>
FORCEINLINE int64 TCString<WIDECHAR>::Atoi64(const WIDECHAR* String) noexcept
{
    return Strtoi64(String, nullptr, 10);
}

template<>
FORCEINLINE float TCString<WIDECHAR>::Atof(const WIDECHAR* String) noexcept
{
    return Strtof(String, nullptr);
}

template<>
FORCEINLINE double TCString<WIDECHAR>::Atod(const WIDECHAR* String) noexcept
{
    return Strtod(String, nullptr);
}

template<>
FORCEINLINE const WIDECHAR* TCString<WIDECHAR>::Empty() noexcept
{
    return L"";
}
