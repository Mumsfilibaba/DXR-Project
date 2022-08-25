#pragma once
#include "AddPointer.h"
#include "ObjectHandling.h"

#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformString.h"

#include <cstring>
#include <cstdarg>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TChar

template<typename InCharType>
struct TChar 
{
    using CHARTYPE         = InCharType;
    using PointerType      = typename TAddPointer<CHARTYPE>::Type;
    using ConstPointerType = const PointerType;

    static CONSTEXPR CHARTYPE Zero = 0;

    static NODISCARD FORCEINLINE CHARTYPE ToUpper(CHARTYPE Char)
    {
        return FPlatformString::ToUpper(Char);
    }

    static NODISCARD FORCEINLINE CHARTYPE ToLower(CHARTYPE Char)
    {
        return FPlatformString::ToLower(Char);
    }

    static NODISCARD FORCEINLINE bool IsSpace(CHARTYPE Char)
    {
        return FPlatformString::IsSpace(Char);
    }
    
    static NODISCARD FORCEINLINE bool IsUpper(CHARTYPE Char)
    {
        return FPlatformString::IsUpper(Char);
    }
    
    static NODISCARD FORCEINLINE bool IsLower(CHARTYPE Char)
    {
        return FPlatformString::IsLower(Char);
    }
    
    static NODISCARD FORCEINLINE bool IsAlnum(CHARTYPE Char)
    {
        return FPlatformString::IsAlnum(Char);
    }
    
    static NODISCARD FORCEINLINE bool IsDigit(CHARTYPE Char)
    {
        return FPlatformString::IsDigit(Char);
    }
    
    static NODISCARD FORCEINLINE bool IsAlpha(CHARTYPE Char)
    {
        return FPlatformString::IsAlpha(Char);
    }
    
    static NODISCARD FORCEINLINE bool IsPunct(CHARTYPE Char)
    {
        return FPlatformString::IsPunct(Char);
    }
    
    static NODISCARD FORCEINLINE bool IsHexDigit(CHARTYPE Char)
    {
        return FPlatformString::IsHexDigit(Char);
    }

    static NODISCARD FORCEINLINE bool IsZero(CHARTYPE Char) 
    { 
        return (Char == Zero); 
    }
};

typedef TChar<CHAR>     FChar;
typedef TChar<WIDECHAR> FCharWide;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TCString

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

    static FORCEINLINE const CHARTYPE* Strstr(const CHARTYPE* String, const CHARTYPE* Substring) noexcept
    {
        return FPlatformString::Strstr(String, Substring);
    }

    static FORCEINLINE const CHARTYPE* Strpbrk(const CHARTYPE* String, const CHARTYPE* Set) noexcept
    {
        return FPlatformString::Strstr(String, Set);
    }

    static FORCEINLINE const CHARTYPE* Strchr(const CHARTYPE* String, CHARTYPE Char) noexcept
    {
        return FPlatformString::Strstr(String, Char);
    }

    static FORCEINLINE const CHARTYPE* Strrchr(const CHARTYPE* String, CHARTYPE Char) noexcept
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

public: 
    
    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // Non standard functions
    
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

    static FORCEINLINE const CHARTYPE* Empty() noexcept;
};

typedef TCString<CHAR>     FCString;
typedef TCString<WIDECHAR> FCStringWide;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCString

template<>
FORCEINLINE const CHAR* TCString<CHAR>::Empty() noexcept
{
    return "";
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCStringWide

template<>
FORCEINLINE const WIDECHAR* TCString<WIDECHAR>::Empty() noexcept
{
    return L"";
}
