#pragma once
#include "Windows.h"

#include "Core/Core.h"

#include "CoreApplication/Generic/GenericApplicationMisc.h"

#ifdef MessageBox
    #undef MessageBox
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsApplicationMisc

class COREAPPLICATION_API CWindowsApplicationMisc final : public CGenericApplicationMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericApplicationMisc Interface

    static FORCEINLINE void MessageBox(const String& Title, const String& Message)
    {
        MessageBoxA(0, Message.CStr(), Title.CStr(), MB_ICONERROR | MB_OK);
    }

    static FORCEINLINE void RequestExit(int32 ExitCode)
    {
        PostQuitMessage(ExitCode);
    }

    static void PumpMessages(bool bUntilEmpty);

    static SModifierKeyState GetModifierKeyState();
};
