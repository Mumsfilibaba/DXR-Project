#pragma once

#if PLATFORM_WINDOWS
#include "Windows.h"

#include "Core/CoreModule.h"

#include "CoreApplication/Interface/PlatformApplicationMisc.h"

#ifdef MessageBox
#undef MessageBox
#endif

class COREAPPLICATION_API CWindowsApplicationMisc final : public CPlatformApplicationMisc
{
public:

    /* Shows a message box with desired title and message */
    static FORCEINLINE void MessageBox(const CString& Title, const CString& Message)
    {
        MessageBoxA(0, Message.CStr(), Title.CStr(), MB_ICONERROR | MB_OK);
    }

    /* Sends a quit message which then is handled in the applications message loop */
    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        PostQuitMessage(ExitCode);
    }

    /* Pumps the application's message queue */
    static void PumpMessages(bool bUntilEmpty);

    /* Retrieves the current modifier key- state */
    static SModifierKeyState GetModifierKeyState();
};

#endif