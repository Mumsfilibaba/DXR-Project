#pragma once
#include "TypeTraits.h"
#include "ObjectHandling.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformString.h"

template<typename InCharType>
struct TCharTraits 
{
    using CharType = InCharType;

    using PointerType      = typename TAddPointer<CharType>::Type;
    using ConstPointerType = const PointerType;

    NODISCARD static FORCEINLINE CharType ToUpper(CharType Char)
    {
        return FPlatformString::ToUpper(Char);
    }

    NODISCARD static FORCEINLINE CharType ToLower(CharType Char)
    {
        return FPlatformString::ToLower(Char);
    }

    NODISCARD static FORCEINLINE bool IsWhitespace(CharType Char)
    {
        return FPlatformString::IsWhitespace(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsUpper(CharType Char)
    {
        return FPlatformString::IsUpper(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsLower(CharType Char)
    {
        return FPlatformString::IsLower(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsAlnum(CharType Char)
    {
        return FPlatformString::IsAlnum(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsDigit(CharType Char)
    {
        return FPlatformString::IsDigit(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsAlpha(CharType Char)
    {
        return FPlatformString::IsAlpha(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsPunct(CharType Char)
    {
        return FPlatformString::IsPunct(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsHexDigit(CharType Char)
    {
        return FPlatformString::IsHexDigit(Char);
    }
};

typedef TCharTraits<CHAR>     FCharTraits;
typedef TCharTraits<WIDECHAR> FCharTraitsWide;

template<typename InCharType>
struct TCString
{
    typedef InCharType CharType;

public:

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(CharType* Buffer, SIZE_T BufferSize, const CharType* Format, ArgTypes&&... Args) noexcept
    {
        return FPlatformString::Snprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...);
    }

    static FORCEINLINE CharType* Strstr(const CharType* String, const CharType* Substring) noexcept
    {
        return FPlatformString::Strstr(String, Substring);
    }

    static FORCEINLINE CharType* Strpbrk(const CharType* String, const CharType* Set) noexcept
    {
        return FPlatformString::Strpbrk(String, Set);
    }

    static FORCEINLINE CharType* Strchr(const CharType* String, CharType Char) noexcept
    {
        return FPlatformString::Strchr(String, Char);
    }

    static FORCEINLINE CharType* Strrchr(const CharType* String, CharType Char) noexcept
    {
        return FPlatformString::Strrchr(String, Char);
    }

    static FORCEINLINE int32 Strlen(const CharType* String) noexcept
    {
        return (String != nullptr) ? FPlatformString::Strlen(String) : 0;
    }

    static FORCEINLINE int32 Strspn(const CharType* String, const CharType* Set) noexcept
    {
        return FPlatformString::Strspn(String, Set);
    }

    static FORCEINLINE CharType* Strcpy(CharType* Dst, const CharType* Src) noexcept
    {
        return FPlatformString::Strcpy(Dst, Src);
    }

    static FORCEINLINE CharType* Strncpy(CharType* Dst, const CharType* Src, SIZE_T InLength) noexcept
    {
        return FPlatformString::Strncpy(Dst, Src, InLength);
    }

    static FORCEINLINE CharType* Strcat(CharType* Dst, const CharType* Src) noexcept
    {
        return FPlatformString::Strcat(Dst, Src);
    }

    static FORCEINLINE CharType* Strncat(CharType* Dst, const CharType* Src, SIZE_T InLength) noexcept
    {
        return FPlatformString::Strncat(Dst, Src, InLength);
    }

public:

    static FORCEINLINE CharType* Strmove(CharType* Dst, const CharType* Src) noexcept
    {
        return Strnmove(Dst, Src, Strlen(Src));
    }

    static FORCEINLINE CharType* Strnmove(CharType* Dst, const CharType* Src, SIZE_T InLength) noexcept
    {
        return reinterpret_cast<CharType*>(FMemory::Memmove(Dst, Src, InLength * sizeof(CharType)));
    }

    static FORCEINLINE CharType* Strset(CharType* Dst, CharType Char) noexcept
    {
        return Strnset(Dst, Char, Strlen(Dst));
    }

    static FORCEINLINE CharType* Strnset(CharType* Dst, CharType Char, SIZE_T InLength) noexcept
    {
        return reinterpret_cast<CharType*>(::AssignObjectsAndReturn(Dst, Char, InLength));
    }

public:

    static FORCEINLINE int32 Strcmp(const CharType* LHS, const CharType* RHS) noexcept
    {
        return FPlatformString::Strcmp(LHS, RHS);
    }

    static FORCEINLINE int32 Strncmp(const CharType* LHS, const CharType* RHS, SIZE_T InLength) noexcept
    {
        return FPlatformString::Strncmp(LHS, RHS, InLength);
    }

    static FORCEINLINE int32 Stricmp(const CharType* LHS, const CharType* RHS) noexcept
    {
        return FPlatformString::Stricmp(LHS, RHS);
    }

    static FORCEINLINE int32 Strnicmp(const CharType* LHS, const CharType* RHS, SIZE_T InLength) noexcept
    {
        return FPlatformString::Strnicmp(LHS, RHS, InLength);
    }

    static FORCEINLINE int32 Strtoi(const CharType* String, CharType** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoi(String, End, Base);
    }
    
    static FORCEINLINE int64 Strtoi64(const CharType* String, CharType** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoi64(String, End, Base);
    }
    
    static FORCEINLINE uint32 Strtoui(const CharType* String, CharType** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoui(String, End, Base);
    }
    
    static FORCEINLINE uint64 Strtoui64(const CharType* String, CharType** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoui64(String, End, Base);
    }
    
    static FORCEINLINE float Strtof(const CharType* String, CharType** End) noexcept
    {
        return FPlatformString::Strtof(String, End);
    }

    static FORCEINLINE double Strtod(const CharType* String, CharType** End) noexcept
    {
        return FPlatformString::Strtod(String, End);
    }
    
    static FORCEINLINE int32 Atoi(const CharType* String) noexcept
    {
        return FPlatformString::Atoi(String);
    }
    
    static FORCEINLINE int64 Atoi64(const CharType* String) noexcept
    {
        return FPlatformString::Atoi64(String);
    }
    
    static FORCEINLINE float Atof(const CharType* String) noexcept
    {
        return FPlatformString::Atof(String);
    }

    static FORCEINLINE double Atod(const CharType* String) noexcept
    {
        return FPlatformString::Atod(String);
    }

    NODISCARD static FORCEINLINE const CharType* Empty() noexcept;
};

template<>
FORCEINLINE const CHAR* TCString<CHAR>::Empty() noexcept
{
    return "";
}

template<>
FORCEINLINE const WIDECHAR* TCString<WIDECHAR>::Empty() noexcept
{
    return L"";
}

typedef TCString<CHAR>     FCString;
typedef TCString<WIDECHAR> FCStringWide;

#define DECLARE_FORMAT_STRING_SPECIFIER(Type, Specifier) \
template<> \
NODISCARD FORCEINLINE decltype(auto) TFormatSpecifier<Type>::GetStringSpecifier() \
{ \
    return Specifier; \
} \
template<> \
NODISCARD FORCEINLINE decltype(auto) TFormatSpecifierWide<Type>::GetStringSpecifier() \
{ \
    return L##Specifier; \
}

template<typename T>
struct TFormatSpecifier
{
    NODISCARD static FORCEINLINE decltype(auto) GetStringSpecifier() 
    { 
        return ""; 
    }
};

template<typename T>
struct TFormatSpecifierWide
{
    NODISCARD static FORCEINLINE decltype(auto) GetStringSpecifier()
    {
        return L"";
    }
};

DECLARE_FORMAT_STRING_SPECIFIER(bool, "%d");

DECLARE_FORMAT_STRING_SPECIFIER(int8, "%d");
DECLARE_FORMAT_STRING_SPECIFIER(int16, "%d");
DECLARE_FORMAT_STRING_SPECIFIER(int32, "%d");
DECLARE_FORMAT_STRING_SPECIFIER(int64, "%lld");

DECLARE_FORMAT_STRING_SPECIFIER(uint8, "%u");
DECLARE_FORMAT_STRING_SPECIFIER(uint16, "%u");
DECLARE_FORMAT_STRING_SPECIFIER(uint32, "%u");
DECLARE_FORMAT_STRING_SPECIFIER(uint64, "%llu");

DECLARE_FORMAT_STRING_SPECIFIER(float, "%f");
DECLARE_FORMAT_STRING_SPECIFIER(double, "%f");
DECLARE_FORMAT_STRING_SPECIFIER(long double, "%f");