#pragma once
#include "Core/Generic/GenericPlatformString.h"

struct FWindowsPlatformString final : public FGenericPlatformString
{
    using FGenericPlatformString::Strtoui64;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CHAR

    NODISCARD static FORCEINLINE int32 Stricmp(const CHAR* String0, const CHAR* String1) noexcept
    {
        return static_cast<int32>(::_stricmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const CHAR* String0, const CHAR* String1, SIZE_T InLength) noexcept
    {
        return static_cast<int32>(::_strnicmp(String0, String1, InLength));
    }

    NODISCARD static FORCEINLINE int64 Strtoi64(const CHAR* String, CHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::_strtoi64(String, End, Base));
    }

    NODISCARD static FORCEINLINE uint64 Strtoui64(const CHAR* String, CHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::_strtoui64(String, End, Base));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // WIDECHAR

    NODISCARD static FORCEINLINE int32 Stricmp(const WIDECHAR* String0, const WIDECHAR* String1) noexcept
    {
        return static_cast<int32>(::_wcsicmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const WIDECHAR* String0, const WIDECHAR* String1, SIZE_T InLength) noexcept
    {
        return static_cast<int32>(::_wcsnicmp(String0, String1, InLength));
    }

    NODISCARD static FORCEINLINE int64 Strtoi64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::_wcstoi64(String, End, Base));
    }

    NODISCARD static FORCEINLINE uint64 Strtoui64(const WIDECHAR* String, WIDECHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::_wcstoui64(String, End, Base));
    }

    NODISCARD static FORCEINLINE int32 Atoi(const WIDECHAR* String) noexcept
    {
        return static_cast<int32>(::_wtoi(String));
    }

    NODISCARD static FORCEINLINE int64 Atoi64(const WIDECHAR* String) noexcept
    {
        return static_cast<int64>(::_wtoi64(String));
    }

    NODISCARD static FORCEINLINE float Atof(const WIDECHAR* String) noexcept
    {
        return static_cast<float>(::_wtof(String));
    }

    NODISCARD static FORCEINLINE double Atod(const WIDECHAR* String) noexcept
    {
        return static_cast<double>(::_wtof(String));
    }
};