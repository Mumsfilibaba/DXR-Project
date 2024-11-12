#pragma once
#include "Core/Generic/GenericPlatformString.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMacPlatformString final : public FGenericPlatformString
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CHAR
    
    NODISCARD static FORCEINLINE int32 Stricmp(const CHAR* String0, const CHAR* String1) noexcept
    {
        return static_cast<int32>(::strcasecmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const CHAR* String0, const CHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::strncasecmp(String0, String1, InLength));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // WIDECHAR

    NODISCARD static FORCEINLINE int32 Stricmp(const WIDECHAR* String0, const WIDECHAR* String1) noexcept
    {
        return static_cast<int32>(::wcscasecmp(String0, String1));
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const WIDECHAR* String0, const WIDECHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::wcsncasecmp(String0, String1, InLength));
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
