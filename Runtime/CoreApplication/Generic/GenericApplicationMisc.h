#pragma once 
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/String.h"

#ifdef MessageBox
    #undef MessageBox
#endif

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericApplication;
struct FOutputDeviceConsole;

struct COREAPPLICATION_API FGenericApplicationMisc
{
    static FGenericApplication*  CreateApplication();
    static FOutputDeviceConsole* CreateOutputDeviceConsole() { return nullptr; }

    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message) { }

    static FORCEINLINE void RequestExit(int32 ExitCode) { }

    static FORCEINLINE void PumpMessages(bool bUntilEmpty) { }

    static FORCEINLINE FModifierKeyState GetModifierKeyState() { return FModifierKeyState(); }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
