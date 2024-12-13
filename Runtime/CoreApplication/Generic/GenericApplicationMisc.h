#pragma once 
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"

#ifdef MessageBox
    #undef MessageBox
#endif

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericApplication;
struct FOutputDeviceConsole;

struct FGenericApplicationMisc
{
    static FOutputDeviceConsole* CreateOutputDeviceConsole() { return nullptr; }

    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message) { }
    static FORCEINLINE void RequestExit(int32 ExitCode) { }
    static FORCEINLINE void PumpMessages(bool bUntilEmpty) { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
