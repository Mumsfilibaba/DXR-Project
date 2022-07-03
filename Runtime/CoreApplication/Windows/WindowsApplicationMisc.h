#pragma once
#include "Windows.h"

#include "Core/Core.h"

#include "CoreApplication/Generic/GenericApplicationMisc.h"

#ifdef MessageBox
    #undef MessageBox
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsApplicationMisc

class COREAPPLICATION_API FWindowsApplicationMisc final : public FGenericApplicationMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericApplicationMisc Interface

    static class FGenericApplication* CreateApplication();

    static class FGenericConsoleWindow* CreateConsoleWindow();

    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message)
    {
        MessageBoxA(0, Message.CStr(), Title.CStr(), MB_ICONERROR | MB_OK);
    }

    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        PostQuitMessage(ExitCode);
    }

    static void PumpMessages(bool bUntilEmpty);

    static FModifierKeyState GetModifierKeyState();
};
