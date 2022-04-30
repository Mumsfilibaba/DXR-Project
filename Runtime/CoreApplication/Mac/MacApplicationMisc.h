#pragma once
#include "CoreApplication/Generic/GenericApplicationMisc.h"

#if defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <AppKit/AppKit.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacApplicationMisc

class CMacApplicationMisc final : public CGenericApplicationMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericApplicationMisc Interface

    static class CGenericApplication* CreateApplication();

    static class CGenericConsoleWindow* CreateConsoleWindow();

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
