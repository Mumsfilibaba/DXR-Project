#pragma once
#include "Core/Windows/Windows.h"
#include "CoreApplication/Generic/GenericApplicationMisc.h"

struct COREAPPLICATION_API FWindowsApplicationMisc final : public FGenericApplicationMisc
{
    static FORCEINLINE void MessageBox(const String& Title, const String& Message)
    {
        MessageBoxA(0, *Message, *Title, MB_ICONERROR | MB_OK);
    }

    static void PumpMessages(bool bUntilEmpty);
};
