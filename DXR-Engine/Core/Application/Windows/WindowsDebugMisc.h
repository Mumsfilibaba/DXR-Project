#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Containers/String.h"
#include "Core/Application/Generic/GenericDebugMisc.h"

#include "Windows.h"

#ifdef MessageBox
#undef MessageBox
#endif

#ifdef OutputDebugString
#undef OutputDebugString
#endif

class CWindowsDebugMisc : public CGenericDebugMisc
{
public:

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
        __debugbreak();
    }

    /* Outputs a debug string to the attached debugger */
    static FORCEINLINE void OutputDebugString( const std::string& Message )
    {
        OutputDebugStringA( Message.c_str() );
    }

    /* Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }

    /* Calls GetLastError and retrives a string from it */
    static void GetLastErrorString( CString& OutErrorString );

};

#endif