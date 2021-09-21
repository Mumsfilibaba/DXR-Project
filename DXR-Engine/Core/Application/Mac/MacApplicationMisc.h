#pragma once

#if defined(PLATFORM_MACOS) 
#include "Core/Application/Generic/GenericApplicationMisc.h"

#if defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class CMacApplicationMisc : public CGenericApplicationMisc
{
public:

    /* Takes the title of the messagebox and the message to be displayed */
    static void MessageBox( const std::string& Title, const std::string& Message );

    /* Sends a Exit Message to the application with a certain exitcode */
    static void RequestExit( int32 ExitCode );

    /* Pumps the application's message queue */
    static void PumpMessages( bool UntilEmpty );

    // TODO: Fix the modifier keys 
};

#if defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

#endif
