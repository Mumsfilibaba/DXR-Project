#pragma once
#include "Core/Windows/Windows.h"
#include "CoreApplication/PlatformInterface/IPlatformApplicationMisc.h"

struct COREAPPLICATION_API FWindowsApplicationMisc final : public IPlatformApplicationMisc
{
    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message)
    {
        MessageBoxA(0, *Message, *Title, MB_ICONERROR | MB_OK);
    }

    static void PumpMessages(bool bUntilEmpty);
    static bool SupportsRoundedWindowCorners();
};
