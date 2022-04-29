#pragma once
#include "CoreApplication/Generic/GenericDebugMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacDebugMisc

class CMacDebugMisc final : public CGenericDebugMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericDebugMisc Interface

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
