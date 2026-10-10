#pragma once
#include "Core/PlatformInterface/IPlatformString.h"

// MSVC reads %s and %c in a wide format as wide, which is what the engine relies on. This define switches it to standard C.
#if defined(_CRT_STDIO_ISO_WIDE_SPECIFIERS)
    #error "_CRT_STDIO_ISO_WIDE_SPECIFIERS changes what %s and %c mean in wide formats"
#endif

struct FWindowsPlatformString final : public IPlatformString
{
    using IPlatformString::Strtoui64;
    using IPlatformString::Atoi;
    using IPlatformString::Atoi64;
    using IPlatformString::Atof;
    using IPlatformString::Atod;

    // -------------------------------------------------------------------------------------------
    // ANSICHAR
    // -------------------------------------------------------------------------------------------

    NODISCARD static FORCEINLINE int32 Stricmp(const ANSICHAR* String0, const ANSICHAR* String1) noexcept
    {
        return static_cast<int32>(::_stricmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const ANSICHAR* String0, const ANSICHAR* String1, SIZE_T InLength) noexcept
    {
        return static_cast<int32>(::_strnicmp(String0, String1, InLength));
    }

    NODISCARD static FORCEINLINE int64 Strtoi64(const ANSICHAR* String, ANSICHAR** End, int32 Base) noexcept
    {
        return static_cast<int64>(::_strtoi64(String, End, Base));
    }

    NODISCARD static FORCEINLINE uint64 Strtoui64(const ANSICHAR* String, ANSICHAR** End, int32 Base) noexcept
    {
        return static_cast<uint64>(::_strtoui64(String, End, Base));
    }

    // -------------------------------------------------------------------------------------------
    // WIDECHAR
    // -------------------------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------------------------
    // UTF8CHAR
    // -------------------------------------------------------------------------------------------

    NODISCARD static FORCEINLINE int32 Stricmp(const UTF8CHAR* String0, const UTF8CHAR* String1) noexcept
    {
        return Stricmp(AsAnsi(String0), AsAnsi(String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const UTF8CHAR* String0, const UTF8CHAR* String1, SIZE_T InLength) noexcept
    {
        return Strnicmp(AsAnsi(String0), AsAnsi(String1), InLength);
    }

    NODISCARD static FORCEINLINE int64 Strtoi64(const UTF8CHAR* String, UTF8CHAR** End, int32 Base) noexcept
    {
        return Strtoi64(AsAnsi(String), AsAnsi(End), Base);
    }

    NODISCARD static FORCEINLINE uint64 Strtoui64(const UTF8CHAR* String, UTF8CHAR** End, int32 Base) noexcept
    {
        return Strtoui64(AsAnsi(String), AsAnsi(End), Base);
    }
};
