#pragma once
#include "CoreApplication/Generic/GenericApplicationMisc.h"

#include <AppKit/AppKit.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

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

#pragma clang diagnostic pop
