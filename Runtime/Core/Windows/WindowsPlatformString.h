#pragma once
#include "Core/Generic/GenericPlatformString.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsPlatformString

struct FWindowsPlatformString : public FGenericPlatformString
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CHAR
    
    static NODISCARD FORCEINLINE int32 Stricmp(const CHAR* String0, const CHAR* String1) noexcept
    {
        return static_cast<int32>(::_stricmp(String0, String1));
    }

    static NODISCARD FORCEINLINE int32 Strnicmp(const CHAR* String0, const CHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::_strnicmp(String0, String1, InLength));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // WIDECHAR

    template<typename... ArgTypes>
    static FORCEINLINE int32 Sprintf(WIDECHAR* Buffer, const WIDECHAR* Format, ArgTypes&&... Args) noexcept
    {
        return static_cast<int32>(::_swprintf(Buffer, Format, Forward<ArgTypes>(Args)...));
    }

    static NODISCARD FORCEINLINE int32 Stricmp(const WIDECHAR* String0, const WIDECHAR* String1) noexcept
    {
        return static_cast<int32>(::_wcsicmp(String0, String1));
    }

    static NODISCARD FORCEINLINE int32 Strnicmp(const WIDECHAR* String0, const WIDECHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::_wcsnicmp(String0, String1, InLength));
    }

    static NODISCARD FORCEINLINE int64 Strtoi64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::_wcstoi64(String, End, Base));
    }

    static NODISCARD FORCEINLINE uint64 Strtoui64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::_wcstoui64(String, End, Base));
    }

    static NODISCARD FORCEINLINE int32 Atoi(const WIDECHAR* String) noexcept
    {
        return static_cast<int32>(::_wtoi(String));
    }

    static NODISCARD FORCEINLINE int64 Atoi64(const WIDECHAR* String) noexcept
    {
        return static_cast<int64>(::_wtoi64(String));
    }

    static NODISCARD FORCEINLINE float Atof(const WIDECHAR* String) noexcept
    {
        return static_cast<float>(::_wtof(String));
    }

    static NODISCARD FORCEINLINE double Atod(const WIDECHAR* String) noexcept
    {
        return static_cast<double>(::_wtof(String));
    }
};