#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Windows.h"

#include "Core/CoreModule.h"
#include "Core/Containers/String.h"

#include "CoreApplication/Interface/PlatformDebugMisc.h"

#ifdef MessageBox
#undef MessageBox
#endif

#ifdef OutputDebugString
#undef OutputDebugString
#endif

class COREAPPLICATION_API CWindowsDebugMisc final : public CPlatformDebugMisc
{
public:

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
        __debugbreak();
    }

    /* Outputs a debug string to the attached debugger */
    static FORCEINLINE void OutputDebugString( const CString& Message )
    {
        OutputDebugStringA( Message.CStr() );
    }

    /* Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }

    /* Calls GetLastError and retrieves a string from it */
    static void GetLastErrorString( CString& OutErrorString );

};

#endif