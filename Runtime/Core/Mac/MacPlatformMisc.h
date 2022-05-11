#pragma once
#include "Core/Generic/GenericPlatformMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacPlatformMisc

class CMacPlatformMisc final : public CGenericPlatformMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericPlatformMisc Interface

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
