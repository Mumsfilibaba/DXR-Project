#pragma once 
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformApplicationMisc
{
    static FORCEINLINE void MessageBox(const String& Title, const String& Message) { }
    static FORCEINLINE void PumpMessages(bool bUntilEmpty) { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
