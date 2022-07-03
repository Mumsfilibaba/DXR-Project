#pragma once 
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/String.h"

#include "CoreApplication/CoreApplication.h"

#ifdef MessageBox
    #undef MessageBox
#endif

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

// TODO: Enable other types of Modal windows for supported platforms

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericApplicationMisc

class COREAPPLICATION_API FGenericApplicationMisc
{
public:

    static class FGenericApplication* CreateApplication();

    static class FGenericConsoleWindow* CreateConsoleWindow();

    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message) { }

    static FORCEINLINE void RequestExit(int32 ExitCode) { }

    static FORCEINLINE void PumpMessages(bool bUntilEmpty) { }

    static FORCEINLINE FModifierKeyState GetModifierKeyState() { return FModifierKeyState(); }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
