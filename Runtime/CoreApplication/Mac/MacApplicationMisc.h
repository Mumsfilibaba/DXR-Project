#pragma once

#if PLATFORM_MACOS 
#include "CoreApplication/Interface/PlatformApplicationMisc.h"

#if defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class CMacApplicationMisc final : public CPlatformApplicationMisc
{
public:

    /* Takes the title of the messagebox and the message to be displayed */
    static void MessageBox( const CString& Title, const CString& Message );

    /* Sends a Exit Message to the application with a certain exitcode */
    static FORCEINLINE void RequestExit( int32 ExitCode )
    {
        [NSApp terminate:nil];
    }

    /* Pumps the application's message queue */
    static void PumpMessages( bool UntilEmpty );

    /* Retrieves the state of modifer keys */
    static SModifierKeyState GetModifierKeyState();
};

#if defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

#endif
