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
public:
    using CharType         = InCharType;
    using PointerType      = typename TAddPointer<CharType>::Type;
    using ConstPointerType = const PointerType;

    static CONSTEXPR InCharType Terminator = 0;

public:
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
NODISCARD FORCEINLINE CHAR 
TChar<CHAR>::ToUpper(CHAR Char) { return static_cast<CHAR>(toupper(Char)); }

template<>
NODISCARD FORCEINLINE CHAR 
TChar<CHAR>::ToLower(CHAR Char) { return static_cast<CHAR>(tolower(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsSpace(CHAR Char) { return static_cast<bool>(isspace(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsUpper(CHAR Char) { return static_cast<bool>(isupper(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsLower(CHAR Char) { return static_cast<bool>(islower(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsAlnum(CHAR Char) { return static_cast<bool>(isalnum(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsDigit(CHAR Char) { return static_cast<bool>(isdigit(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsAlpha(CHAR Char) { return static_cast<bool>(isalpha(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsPunct(CHAR Char) { return static_cast<bool>(ispunct(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<CHAR>::IsHexDigit(CHAR Char) { return static_cast<bool>(isxdigit(Char)); }

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCharWide

template<>
NODISCARD FORCEINLINE WIDECHAR 
TChar<WIDECHAR>::ToUpper(WIDECHAR Char) { return static_cast<WIDECHAR>(towupper(Char)); }

template<>
NODISCARD FORCEINLINE WIDECHAR 
TChar<WIDECHAR>::ToLower(WIDECHAR Char) { return static_cast<WIDECHAR>(towlower(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsSpace(WIDECHAR Char) { return static_cast<bool>(iswspace(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsUpper(WIDECHAR Char) { return static_cast<bool>(iswupper(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsLower(WIDECHAR Char) { return static_cast<bool>(iswlower(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsAlnum(WIDECHAR Char) { return static_cast<bool>(iswalnum(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsDigit(WIDECHAR Char) { return static_cast<bool>(iswdigit(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsAlpha(WIDECHAR Char) { return static_cast<bool>(iswalpha(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsPunct(WIDECHAR Char) { return static_cast<bool>(iswpunct(Char)); }

template<>
NODISCARD FORCEINLINE bool
TChar<WIDECHAR>::IsHexDigit(WIDECHAR Char) { return static_cast<bool>(iswxdigit(Char)); }

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TCString

template<typename CharType>
struct TCString
{
    static NODISCARD FORCEINLINE const CharType* Find(const CharType* String, const CharType* Substring) noexcept;
    static NODISCARD FORCEINLINE const CharType* FindOneOf(const CharType* String, const CharType* Set) noexcept;

    static NODISCARD FORCEINLINE const CharType* FindChar(const CharType* String, CharType Char) noexcept;
    static NODISCARD FORCEINLINE const CharType* ReverseFindChar(const CharType* String, CharType Char) noexcept;

    static NODISCARD FORCEINLINE int32 Length(const CharType* String) noexcept;
    static NODISCARD FORCEINLINE int32 RangeLength(const CharType* String, const CharType* Set) noexcept;

    static NOINLINE    int32 FormatBuffer(CharType* Buffer, int32 BufferLength, const CharType* Format, ...) noexcept;
    static FORCEINLINE int32 FormatBufferV(CharType* Buffer, int32 Len, const CharType* Format, va_list Args) noexcept;

    static FORCEINLINE CharType* Copy(CharType* Dest, const CharType* Source) noexcept;
    static FORCEINLINE CharType* Copy(CharType* Dest, const CharType* Source, uint64 InLength) noexcept;
    
    static FORCEINLINE CharType* Move(CharType* Dest, const CharType* Source) noexcept;
    static FORCEINLINE CharType* Move(CharType* Dest, const CharType* Source, uint64 InLength) noexcept;
    
    static NODISCARD FORCEINLINE int32 Compare(const CharType* LHS, const CharType* RHS) noexcept;
    static NODISCARD FORCEINLINE int32 Compare(const CharType* LHS, const CharType* RHS, uint64 InLength) noexcept;

    static NODISCARD FORCEINLINE int8  ParseInt8(const CharType* String, CharType** End, int32 Base)  noexcept;
    static NODISCARD FORCEINLINE int16 ParseInt16(const CharType* String, CharType** End, int32 Base) noexcept;
    static NODISCARD FORCEINLINE int32 ParseInt32(const CharType* String, CharType** End, int32 Base) noexcept;
    static NODISCARD FORCEINLINE int64 ParseInt64(const CharType* String, CharType** End, int32 Base) noexcept;

    static NODISCARD FORCEINLINE uint8  ParseUint8(const CharType* String, CharType** End, int32 Base)  noexcept;
    static NODISCARD FORCEINLINE uint16 ParseUint16(const CharType* String, CharType** End, int32 Base) noexcept;
    static NODISCARD FORCEINLINE uint32 ParseUint32(const CharType* String, CharType** End, int32 Base) noexcept;
    static NODISCARD FORCEINLINE uint64 ParseUint64(const CharType* String, CharType** End, int32 Base) noexcept;

    static NODISCARD FORCEINLINE float  ParseFloat(const CharType* String, CharType** End)  noexcept;
    static NODISCARD FORCEINLINE double ParseDouble(const CharType* String, CharType** End) noexcept;
    
    static NODISCARD FORCEINLINE const CharType* Empty() noexcept;
};

typedef TCString<CHAR>     FCString;
typedef TCString<WIDECHAR> FCStringWide;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCString

template<>
NODISCARD FORCEINLINE const CHAR* 
TCString<CHAR>::Find(const CHAR* String, const CHAR* Substring) noexcept
{
    return strstr(String, Substring);
}

template<>
NODISCARD FORCEINLINE const CHAR* 
TCString<CHAR>::FindOneOf(const CHAR* String, const CHAR* Set) noexcept
{
    return strpbrk(String, Set);
}

template<>
NODISCARD FORCEINLINE const CHAR* 
TCString<CHAR>::FindChar(const CHAR* String, CHAR Char) noexcept
{
    return strchr(String, Char);
}

template<>
NODISCARD FORCEINLINE const CHAR* 
TCString<CHAR>::ReverseFindChar(const CHAR* String, CHAR Char) noexcept
{
    return strrchr(String, Char);
}

template<>
NODISCARD FORCEINLINE int32
TCString<CHAR>::RangeLength(const CHAR* String, const CHAR* Set) noexcept
{
    return static_cast<int32>(strspn(String, Set));
}

template<>
NODISCARD FORCEINLINE int32 
TCString<CHAR>::Length(const CHAR* String) noexcept
{
    return (String != nullptr) ? static_cast<int32>(strlen(String)) : 0;
}

template<>
FORCEINLINE int32 
TCString<CHAR>::FormatBuffer(CHAR* Buffer, int32 BufferLength, const CHAR* Format, ...) noexcept
{
    va_list Args;
    va_start(Args, Format);
    int32 Result = FormatBufferV(Buffer, BufferLength, Format, Args);
    va_end(Args);

    return Result;
}

template<>
FORCEINLINE int32 
TCString<CHAR>::FormatBufferV(CHAR* Buffer, int32 Len, const CHAR* Format, va_list Args) noexcept
{
    return vsnprintf(Buffer, Len, Format, Args);
}

template<>
FORCEINLINE CHAR* 
TCString<CHAR>::Copy(CHAR* Dest, const CHAR* Source) noexcept
{
    return Copy(Dest, Source, Length(Source));
}

template<>
FORCEINLINE CHAR* 
TCString<CHAR>::Copy(CHAR* Dest, const CHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<CHAR*>(FMemory::Memcpy(Dest, Source, InLength * sizeof(CHAR)));
}

template<>
FORCEINLINE CHAR* 
TCString<CHAR>::Move(CHAR* Dest, const CHAR* Source) noexcept
{
    return Move(Dest, Source, Length(Source));
}

template<>
FORCEINLINE CHAR* 
TCString<CHAR>::Move(CHAR* Dest, const CHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<CHAR*>(FMemory::Memmove(Dest, Source, InLength * sizeof(CHAR)));
}

template<>
NODISCARD FORCEINLINE int32 
TCString<CHAR>::Compare(const CHAR* LHS, const CHAR* RHS) noexcept
{
    return static_cast<int32>(strcmp(LHS, RHS));
}

template<>
NODISCARD FORCEINLINE int32 
TCString<CHAR>::Compare(const CHAR* LHS, const CHAR* RHS, uint64 InLength) noexcept
{
    return static_cast<int32>(strncmp(LHS, RHS, InLength));
}

template<>
NODISCARD FORCEINLINE int8 
TCString<CHAR>::ParseInt8(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<int8>(strtol(String, End, Base));
}

template<>
NODISCARD FORCEINLINE int16 
TCString<CHAR>::ParseInt16(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<int16>(strtol(String, End, Base));
}

template<>
NODISCARD FORCEINLINE int32 
TCString<CHAR>::ParseInt32(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<int32>(strtol(String, End, Base));
}

template<>
NODISCARD FORCEINLINE int64 
TCString<CHAR>::ParseInt64(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<int64>(strtoll(String, End, Base));
}

template<>
NODISCARD FORCEINLINE uint8 
TCString<CHAR>::ParseUint8(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<uint8>(strtoul(String, End, Base));
}

template<>
NODISCARD FORCEINLINE uint16 
TCString<CHAR>::ParseUint16(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<uint16>(strtoul(String, End, Base));
}

template<>
NODISCARD FORCEINLINE uint32 
TCString<CHAR>::ParseUint32(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<uint32>(strtoul(String, End, Base));
}

template<>
NODISCARD FORCEINLINE uint64 
TCString<CHAR>::ParseUint64(const CHAR* String, CHAR** End, int32 Base) noexcept
{
    return static_cast<uint64>(strtoull(String, End, Base));
}

template<>
NODISCARD FORCEINLINE float 
TCString<CHAR>::ParseFloat(const CHAR* String, CHAR** End) noexcept
{
    return static_cast<float>(strtold(String, End));
}

template<>
NODISCARD FORCEINLINE double 
TCString<CHAR>::ParseDouble(const CHAR* String, CHAR** End) noexcept
{
    return static_cast<double>(strtold(String, End));
}

template<>
NODISCARD FORCEINLINE const CHAR*
TCString<CHAR>::Empty() noexcept
{
    return "";
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCStringWide

template<>
NODISCARD FORCEINLINE const WIDECHAR*
TCString<WIDECHAR>::Find(const WIDECHAR* String, const WIDECHAR* Substring) noexcept
{
    return wcsstr(String, Substring);
}

template<>
NODISCARD FORCEINLINE const WIDECHAR*
TCString<WIDECHAR>::FindOneOf(const WIDECHAR* String, const WIDECHAR* Set) noexcept
{
    return wcspbrk(String, Set);
}

template<>
NODISCARD FORCEINLINE const WIDECHAR*
TCString<WIDECHAR>::FindChar(const WIDECHAR* String, WIDECHAR Char) noexcept
{
    return wcschr(String, Char);
}

template<>
NODISCARD FORCEINLINE const WIDECHAR*
TCString<WIDECHAR>::ReverseFindChar(const WIDECHAR* String, WIDECHAR Char) noexcept
{
    return wcsrchr(String, Char);
}

template<>
NODISCARD FORCEINLINE int32
TCString<WIDECHAR>::RangeLength(const WIDECHAR* String, const WIDECHAR* Set) noexcept
{
    return static_cast<int32>(wcsspn(String, Set));
}

template<>
NODISCARD FORCEINLINE int32
TCString<WIDECHAR>::Length(const WIDECHAR* String) noexcept
{
    return (String != nullptr) ? static_cast<int32>(wcslen(String)) : 0;
}

template<>
FORCEINLINE int32
TCString<WIDECHAR>::FormatBuffer(WIDECHAR* Buffer, int32 BufferLength, const WIDECHAR* Format, ...) noexcept
{
    va_list Args;
    va_start(Args, Format);
    int32 Result = FormatBufferV(Buffer, BufferLength, Format, Args);
    va_end(Args);

    return Result;
}

template<>
FORCEINLINE int32
TCString<WIDECHAR>::FormatBufferV(WIDECHAR* Buffer, int32 Len, const WIDECHAR* Format, va_list Args) noexcept
{
    return vswprintf(Buffer, Len, Format, Args);
}

template<>
FORCEINLINE WIDECHAR*
TCString<WIDECHAR>::Copy(WIDECHAR* Dest, const WIDECHAR* Source) noexcept
{
    return Copy(Dest, Source, Length(Source));
}

template<>
FORCEINLINE WIDECHAR*
TCString<WIDECHAR>::Copy(WIDECHAR* Dest, const WIDECHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<WIDECHAR*>(FMemory::Memcpy(Dest, Source, InLength * sizeof(WIDECHAR)));
}

template<>
FORCEINLINE WIDECHAR*
TCString<WIDECHAR>::Move(WIDECHAR* Dest, const WIDECHAR* Source) noexcept
{
    return Move(Dest, Source, Length(Source));
}

template<>
FORCEINLINE WIDECHAR*
TCString<WIDECHAR>::Move(WIDECHAR* Dest, const WIDECHAR* Source, uint64 InLength) noexcept
{
    return reinterpret_cast<WIDECHAR*>(FMemory::Memmove(Dest, Source, InLength * sizeof(WIDECHAR)));
}

template<>
NODISCARD FORCEINLINE int32
TCString<WIDECHAR>::Compare(const WIDECHAR* LHS, const WIDECHAR* RHS) noexcept
{
    return static_cast<int32>(wcscmp(LHS, RHS));
}

template<>
NODISCARD FORCEINLINE int32
TCString<WIDECHAR>::Compare(const WIDECHAR* LHS, const WIDECHAR* RHS, uint64 InLength) noexcept
{
    return static_cast<int32>(wcsncmp(LHS, RHS, InLength));
}

template<>
NODISCARD FORCEINLINE const WIDECHAR*
TCString<WIDECHAR>::Empty() noexcept
{
    return L"";
}
