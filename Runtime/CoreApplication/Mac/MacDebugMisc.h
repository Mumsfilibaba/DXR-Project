#pragma once

#if PLATFORM_MACOS 
#include "CoreApplication/Generic/GenericDebugMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation for miscellaneous debug functions

class CMacDebugMisc final : public CGenericDebugMisc
{
public:

     /** @brief: If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
        __builtin_trap();
    }

     /** @brief: Outputs a debug string to the attached debugger */
    static void OutputDebugString(const String& Message);

     /** @brief: Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        // TODO: Return real value
        return false;
    }
};

#endif