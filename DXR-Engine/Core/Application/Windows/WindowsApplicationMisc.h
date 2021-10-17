#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Windows.h"

#include "Core/CoreAPI.h"
#include "Core/Application/Core/CoreApplicationMisc.h"


#ifdef MessageBox
#undef MessageBox
#endif

class CORE_API CWindowsApplicationMisc : public CCoreApplicationMisc
{
public:

    /* Shows a message box with desired title and message */
    static FORCEINLINE void MessageBox( const CString& Title, const CString& Message )
    {
        MessageBoxA( 0, Message.CStr(), Title.CStr(), MB_ICONERROR | MB_OK );
    }

    /* Sends a quit message which then is handled in the applications message loop */
    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
        PostQuitMessage( ExitCode );
    }

    /* Pumps the application's message queue */
    static void PumpMessages( bool UntilEmpty );

    /* Retrieves the current modifier key- state */
    static SModifierKeyState GetModifierKeyState();
};

#endif