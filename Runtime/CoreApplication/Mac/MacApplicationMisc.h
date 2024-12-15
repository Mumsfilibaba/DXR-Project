#pragma once
#include "CoreApplication/Generic/GenericApplicationMisc.h"
#include <AppKit/AppKit.h>

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct COREAPPLICATION_API FMacApplicationMisc final : public FGenericApplicationMisc
{
    static void MessageBox(const FString& Title, const FString& Message);
    static void PumpMessages(bool bUntilEmpty);
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
