#pragma once
#include "CoreApplication/Generic/GenericApplicationMisc.h"

#include <AppKit/AppKit.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacApplicationMisc

class FMacApplicationMisc final : public FGenericApplicationMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericApplicationMisc Interface

    static class FGenericApplication* CreateApplication();

    static class FGenericConsoleWindow* CreateConsoleWindow();

    static void MessageBox(const String& Title, const String& Message);

    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        [NSApp terminate:nil];
    }

    static void PumpMessages(bool bUntilEmpty);

    static FModifierKeyState GetModifierKeyState();
};

#pragma clang diagnostic pop
