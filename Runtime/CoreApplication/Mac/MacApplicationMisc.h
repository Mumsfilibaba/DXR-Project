#pragma once

#if PLATFORM_MACOS 
#include "CoreApplication/Interface/PlatformApplicationMisc.h"

#if defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <AppKit/AppKit.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacApplicationMisc - Mac specific implementation for miscellaneous application utility functions

class CMacApplicationMisc final : public CPlatformApplicationMisc
{
public:

    static void MessageBox(const String& Title, const String& Message);

    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        [NSApp terminate:nil];
    }

    static void PumpMessages(bool bUntilEmpty);

    static SModifierKeyState GetModifierKeyState();
};

#if defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

#endif
