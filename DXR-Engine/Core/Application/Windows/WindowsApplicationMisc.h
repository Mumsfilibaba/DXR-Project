#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Generic/GenericApplicationMisc.h"

#include "Windows.h"

class CWindowsApplicationMisc : public CGenericApplicationMisc
{
public:
    
    /* Shows a message box with desired title and message */
    static FORCEINLINE void MessageBox( const std::string& Title, const std::string& Message )
    {
        MessageBoxA( 0, Message.c_str(), Title.c_str(), MB_ICONERROR | MB_OK );
    }

    /* Sends a quit message which then is handled in the applications message loop */
    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
        PostQuitMessage( ExitCode );
    }

    /* Pumps the application's message queue */
    static FORCEINLINE void PumpMessages( bool UntilEmpty )
    {
    }
};

#endif