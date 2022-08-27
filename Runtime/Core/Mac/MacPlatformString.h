#pragma once
#include "Core/Generic/GenericPlatformString.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacPlatformString

struct FMacPlatformString : public FGenericPlatformString
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CHAR
    
    NODISCARD
    static FORCEINLINE int32 Stricmp(const CHAR* String0, const CHAR* String1) noexcept
    {
        return static_cast<int32>(::strcasecmp(String0, String1));
    }

    NODISCARD
    static FORCEINLINE int32 Strnicmp(const CHAR* String0, const CHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::strncasecmp(String0, String1, InLength));
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // WIDECHAR

    template<typename... ArgTypes>
    static FORCEINLINE int32 Sprintf(WIDECHAR* Buffer, const WIDECHAR* Format, ArgTypes&&... Args) noexcept
    {
        UNREFERENCED_VARIABLE(Buffer);
        UNREFERENCED_VARIABLE(Format);
        
        // TODO: Finish
        Check(false);
        return 0; 
    }
    
    NODISCARD
    static FORCEINLINE int32 Stricmp(const WIDECHAR* String0, const WIDECHAR* String1) noexcept
    {
        return static_cast<int32>(::wcscasecmp(String0, String1));
    }

    NODISCARD
    static FORCEINLINE int32 Strnicmp(const WIDECHAR* String0, const WIDECHAR* String1, TSIZE InLength) noexcept
    {
        return static_cast<int32>(::wcsncasecmp(String0, String1, InLength));
    }
};

#pragma clang diagnostic pop
