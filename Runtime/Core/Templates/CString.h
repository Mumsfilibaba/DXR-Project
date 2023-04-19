#pragma once
#include "TypeTraits.h"
#include "ObjectHandling.h"

#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformString.h"

template<typename InCharType>
struct TChar 
{
    using CHARTYPE = InCharType;

    using PointerType      = typename TAddPointer<CHARTYPE>::Type;
    using ConstPointerType = const PointerType;

    static CONSTEXPR CHARTYPE Null = 0;

    NODISCARD static FORCEINLINE CHARTYPE ToUpper(CHARTYPE Char)
    {
        return FPlatformString::ToUpper(Char);
    }

    NODISCARD static FORCEINLINE CHARTYPE ToLower(CHARTYPE Char)
    {
        return FPlatformString::ToLower(Char);
    }

    NODISCARD static FORCEINLINE bool IsSpace(CHARTYPE Char)
    {
        return FPlatformString::IsSpace(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsUpper(CHARTYPE Char)
    {
        return FPlatformString::IsUpper(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsLower(CHARTYPE Char)
    {
        return FPlatformString::IsLower(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsAlnum(CHARTYPE Char)
    {
        return FPlatformString::IsAlnum(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsDigit(CHARTYPE Char)
    {
        return FPlatformString::IsDigit(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsAlpha(CHARTYPE Char)
    {
        return FPlatformString::IsAlpha(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsPunct(CHARTYPE Char)
    {
        return FPlatformString::IsPunct(Char);
    }
    
    NODISCARD static FORCEINLINE bool IsHexDigit(CHARTYPE Char)
    {
        return FPlatformString::IsHexDigit(Char);
    }

    NODISCARD static FORCEINLINE bool IsZero(CHARTYPE Char) 
    { 
        return (Char == Null); 
    }
};

typedef TChar<CHAR>     FChar;
typedef TChar<WIDECHAR> FCharWide;

template<typename InCharType>
struct TCString
{
    typedef InCharType CHARTYPE;

    template<typename... ArgTypes>
    static FORCEINLINE int32 Sprintf(CHARTYPE* Buffer, const CHARTYPE* Format, ArgTypes&&... Args) noexcept
    {
        return FPlatformString::Sprintf(Buffer, Format, Forward<ArgTypes>(Args)...);
    }

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(CHARTYPE* Buffer, TSIZE BufferSize, const CHARTYPE* Format, ArgTypes&&... Args) noexcept
    {
        return FPlatformString::Snprintf(Buffer, BufferSize, Format, Forward<ArgTypes>(Args)...);
    }

    static FORCEINLINE CHARTYPE* Strstr(const CHARTYPE* String, const CHARTYPE* Substring) noexcept
    {
        return FPlatformString::Strstr(String, Substring);
    }

    static FORCEINLINE CHARTYPE* Strpbrk(const CHARTYPE* String, const CHARTYPE* Set) noexcept
    {
        return FPlatformString::Strpbrk(String, Set);
    }

    static FORCEINLINE CHARTYPE* Strchr(const CHARTYPE* String, CHARTYPE Char) noexcept
    {
        return FPlatformString::Strchr(String, Char);
    }

    static FORCEINLINE CHARTYPE* Strrchr(const CHARTYPE* String, CHARTYPE Char) noexcept
    {
        return FPlatformString::Strrchr(String, Char);
    }

    static FORCEINLINE int32 Strlen(const CHARTYPE* String) noexcept
    {
        return (String != nullptr) ? FPlatformString::Strlen(String) : 0;
    }

    static FORCEINLINE int32 Strspn(const CHARTYPE* String, const CHARTYPE* Set) noexcept
    {
        return FPlatformString::Strspn(String, Set);
    }

    static FORCEINLINE CHARTYPE* Strcpy(CHARTYPE* Dst, const CHARTYPE* Src) noexcept
    {
        return FPlatformString::Strcpy(Dst, Src);
    }

    static FORCEINLINE CHARTYPE* Strncpy(CHARTYPE* Dst, const CHARTYPE* Src, TSIZE InLength) noexcept
    {
        return FPlatformString::Strncpy(Dst, Src, InLength);
    }

    static FORCEINLINE CHARTYPE* Strcat(CHARTYPE* Dst, const CHARTYPE* Src) noexcept
    {
        return FPlatformString::Strcat(Dst, Src);
    }

    static FORCEINLINE CHARTYPE* Strncat(CHARTYPE* Dst, const CHARTYPE* Src, TSIZE InLength) noexcept
    {
        return FPlatformString::Strncat(Dst, Src, InLength);
    }

public:
    static FORCEINLINE CHARTYPE* Strmove(CHARTYPE* Dst, const CHARTYPE* Src) noexcept
    {
        return Strnmove(Dst, Src, Strlen(Src));
    }

    static FORCEINLINE CHARTYPE* Strnmove(CHARTYPE* Dst, const CHARTYPE* Src, TSIZE InLength) noexcept
    {
        return reinterpret_cast<CHARTYPE*>(FMemory::Memmove(Dst, Src, InLength * sizeof(CHARTYPE)));
    }

    static FORCEINLINE CHARTYPE* Strset(CHARTYPE* Dst, CHARTYPE Char) noexcept
    {
        return Strnset(Dst, Char, Strlen(Dst));
    }

    static FORCEINLINE CHARTYPE* Strnset(CHARTYPE* Dst, CHARTYPE Char, TSIZE InLength) noexcept
    {
        return reinterpret_cast<CHARTYPE*>(::AssignElementsAndReturn(Dst, Char, InLength));
    }

public:
    static FORCEINLINE int32 Strcmp(const CHARTYPE* LHS, const CHARTYPE* RHS) noexcept
    {
        return FPlatformString::Strcmp(LHS, RHS);
    }

    static FORCEINLINE int32 Strncmp(const CHARTYPE* LHS, const CHARTYPE* RHS, TSIZE InLength) noexcept
    {
        return FPlatformString::Strncmp(LHS, RHS, InLength);
    }

    static FORCEINLINE int32 Stricmp(const CHARTYPE* LHS, const CHARTYPE* RHS) noexcept
    {
        return FPlatformString::Stricmp(LHS, RHS);
    }

    static FORCEINLINE int32 Strnicmp(const CHARTYPE* LHS, const CHARTYPE* RHS, TSIZE InLength) noexcept
    {
        return FPlatformString::Strnicmp(LHS, RHS, InLength);
    }

    static FORCEINLINE int32 Strtoi(const CHARTYPE* String, CHARTYPE** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoi(String, End, Base);
    }
    
    static FORCEINLINE int64 Strtoi64(const CHARTYPE* String, CHARTYPE** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoi64(String, End, Base);
    }
    
    static FORCEINLINE uint32 Strtoui(const CHARTYPE* String, CHARTYPE** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoui(String, End, Base);
    }
    
    static FORCEINLINE uint64 Strtoui64(const CHARTYPE* String, CHARTYPE** End, int32 Base) noexcept
    {
        return FPlatformString::Strtoui64(String, End, Base);
    }
    
    static FORCEINLINE float Strtof(const CHARTYPE* String, CHARTYPE** End) noexcept
    {
        return FPlatformString::Strtof(String, End);
    }

    static FORCEINLINE double Strtod(const CHARTYPE* String, CHARTYPE** End) noexcept
    {
        return FPlatformString::Strtod(String, End);
    }
    
    static FORCEINLINE int32 Atoi(const CHARTYPE* String) noexcept
    {
        return FPlatformString::Atoi(String);
    }
    
    static FORCEINLINE int64 Atoi64(const CHARTYPE* String) noexcept
    {
        return FPlatformString::Atoi64(String);
    }
    
    static FORCEINLINE float Atof(const CHARTYPE* String) noexcept
    {
        return FPlatformString::Atof(String);
    }

    static FORCEINLINE double Atod(const CHARTYPE* String) noexcept
    {
        return FPlatformString::Atod(String);
    }

    NODISCARD static FORCEINLINE const CHARTYPE* Empty() noexcept;
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

#define DECLARE_FORMAT_STRING_SPECIFIER(Type, Specifier)                              \
template<>                                                                            \
NODISCARD FORCEINLINE decltype(auto) TFormatSpecifier<Type>::GetStringSpecifier()     \
{                                                                                     \
    return Specifier;                                                                 \
}                                                                                     \
                                                                                      \
template<>                                                                            \
NODISCARD FORCEINLINE decltype(auto) TFormatSpecifierWide<Type>::GetStringSpecifier() \
{                                                                                     \
    return L##Specifier;                                                              \
}

template<typename T>
struct TFormatSpecifier
{
    NODISCARD static FORCEINLINE decltype(auto) GetStringSpecifier() 
    { 
        return  ""; 
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