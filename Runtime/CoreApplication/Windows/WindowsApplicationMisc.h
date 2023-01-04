#pragma once
#include "Core/Core.h"
#include "Core/Windows/Windows.h"
#include "CoreApplication/Generic/GenericApplicationMisc.h"

struct COREAPPLICATION_API FWindowsApplicationMisc final 
    : public FGenericApplicationMisc
{
    static class FGenericApplication* CreateApplication();

    static struct FOutputDeviceConsole* CreateOutputDeviceConsole();

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
