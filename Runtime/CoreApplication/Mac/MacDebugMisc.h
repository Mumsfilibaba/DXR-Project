#pragma once

#if PLATFORM_MACOS 
#include "CoreApplication/Interface/PlatformDebugMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacDebugMisc - Mac specific implementation for miscellaneous debug functions

class CMacDebugMisc final : public CPlatformDebugMisc
{
public:

    static FORCEINLINE void DebugBreak()
    {
        __builtin_trap();
    }

    static void OutputDebugString(const String& Message);

    static FORCEINLINE bool IsDebuggerPresent()
    {
        // TODO: Return real value
        return false;
    }
};

#endif
