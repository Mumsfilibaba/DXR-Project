#pragma once
#include "Core/PlatformInterface/IPlatformString.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMacPlatformString final : public IPlatformString
{
    using IPlatformString::Snprintf;

    template<typename... ArgTypes>
    static FORCEINLINE int32 Snprintf(WIDECHAR* Buffer, SIZE_T BufferSize, const WIDECHAR* Format, ArgTypes&&... Args) noexcept
    {
        static constexpr int32 MaxFormatLength = 1024;

        WIDECHAR StandardFormat[MaxFormatLength];
        if (!ConvertWideFormatToStandard(StandardFormat, MaxFormatLength, Format))
        {
            return -1;
        }

        DISABLE_FORMAT_SECURITY_WARNING
        return static_cast<int32>(::swprintf(Buffer, BufferSize, StandardFormat, Forward<ArgTypes>(Args)...));
        ENABLE_FORMAT_SECURITY_WARNING
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // ANSICHAR

    NODISCARD static FORCEINLINE int32 Stricmp(const ANSICHAR* String0, const ANSICHAR* String1) noexcept
    {
        return static_cast<int32>(::strcasecmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const ANSICHAR* String0, const ANSICHAR* String1, SIZE_T InLength) noexcept
    {
        return static_cast<int32>(::strncasecmp(String0, String1, InLength));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // WIDECHAR

    NODISCARD static FORCEINLINE int32 Stricmp(const WIDECHAR* String0, const WIDECHAR* String1) noexcept
    {
        return static_cast<int32>(::wcscasecmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const WIDECHAR* String0, const WIDECHAR* String1, SIZE_T InLength) noexcept
    {
        return static_cast<int32>(::wcsncasecmp(String0, String1, InLength));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // UTF8CHAR

    NODISCARD static FORCEINLINE int32 Stricmp(const UTF8CHAR* String0, const UTF8CHAR* String1) noexcept
    {
        return Stricmp(AsAnsi(String0), AsAnsi(String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const UTF8CHAR* String0, const UTF8CHAR* String1, SIZE_T InLength) noexcept
    {
        return Strnicmp(AsAnsi(String0), AsAnsi(String1), InLength);
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
