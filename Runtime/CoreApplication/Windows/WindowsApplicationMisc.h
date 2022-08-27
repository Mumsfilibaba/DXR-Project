#pragma once
#include "Windows.h"

#include "Core/Core.h"

#include "CoreApplication/Generic/GenericApplicationMisc.h"

#ifdef MessageBox
    #undef MessageBox
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsApplicationMisc

struct COREAPPLICATION_API FWindowsApplicationMisc final 
    : public FGenericApplicationMisc
{
    static FOutputDeviceConsole* CreateOutputDeviceConsole();
    static FGenericApplication*  CreateApplication();

    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message)
    {
        MessageBoxA(0, Message.GetCString(), Title.GetCString(), MB_ICONERROR | MB_OK);
    }

    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        PostQuitMessage(ExitCode);
    }

    static void PumpMessages(bool bUntilEmpty);

    static FModifierKeyState GetModifierKeyState();
};
