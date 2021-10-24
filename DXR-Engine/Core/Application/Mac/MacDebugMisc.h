#pragma once

#if defined(PLATFORM_MACOS) 
#include "Core/Application/Core/CoreDebugMisc.h"

class CMacDebugMisc : public CCoreDebugMisc
{
public:

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
        __builtin_trap();
    }

    /* Outputs a debug string to the attached debugger */
    static void OutputDebugString( const CString& Message );

    /* Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return false;
    }
};

#endif
