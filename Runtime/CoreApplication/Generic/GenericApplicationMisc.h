#pragma once 
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericApplicationMisc
{
    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message) { }
    static FORCEINLINE void PumpMessages(bool bUntilEmpty) { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
