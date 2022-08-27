#pragma once
#include "CoreApplication/Generic/GenericApplicationMisc.h"

#include <AppKit/AppKit.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacApplicationMisc

struct FMacApplicationMisc final
    : public FGenericApplicationMisc
{
    static class  FGenericApplication*  CreateApplication();
    static struct FOutputDeviceConsole* CreateOutputDeviceConsole();
 
    static void MessageBox(const FString& Title, const FString& Message);

    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        [NSApp terminate:nil];
    }

    static void PumpMessages(bool bUntilEmpty);

    static FModifierKeyState GetModifierKeyState();
};

#pragma clang diagnostic pop
