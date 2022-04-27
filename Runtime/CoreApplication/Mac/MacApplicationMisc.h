#pragma once

#if PLATFORM_MACOS 
#include "CoreApplication/Generic/GenericApplicationMisc.h"

#if defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <AppKit/AppKit.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation for miscellaneous application utility functions

class CMacApplicationMisc final : public CGenericApplicationMisc
{
public:

     /** @brief: Takes the title of the messagebox and the message to be displayed */
    static void MessageBox(const String& Title, const String& Message);

     /** @brief: Sends a Exit Message to the application with a certain exitcode */
    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        [NSApp terminate:nil];
    }

     /** @brief: Pumps the application's message queue */
    static void PumpMessages(bool bUntilEmpty);

     /** @brief: Retrieves the state of modifer keys */
    static SModifierKeyState GetModifierKeyState();
};

#if defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

#endif
