#pragma once 
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"

#ifdef MessageBox
    #undef MessageBox
#endif

enum EModifierFlag : uint8
{
    ModifierFlag_None     = 0,
    ModifierFlag_Ctrl     = FLAG(1),
    ModifierFlag_Alt      = FLAG(2),
    ModifierFlag_Shift    = FLAG(3),
    ModifierFlag_CapsLock = FLAG(4),
    ModifierFlag_Super    = FLAG(5),
    ModifierFlag_NumLock  = FLAG(6),
};

struct FModifierKeyState
{
    FModifierKeyState() = default;

    FModifierKeyState(uint8 InModifierMask)
        : ModifierMask(InModifierMask)
    {
    }

    union
    {
         /** @brief - Flags */
        struct
        {
            uint8 bIsCtrlDown     : 1;
            uint8 bIsAltDown      : 1;
            uint8 bIsShiftDown    : 1;
            uint8 bIsCapsLockDown : 1;
            uint8 bIsSuperDown    : 1;
            uint8 bIsNumPadDown   : 1;
        };

         /** @brief - Mask */
        uint8 ModifierMask = 0;
    };
};

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericApplication;
struct FOutputDeviceConsole;

struct FGenericApplicationMisc
{
    static FOutputDeviceConsole* CreateOutputDeviceConsole() { return nullptr; }

    static FORCEINLINE void MessageBox(const FString& Title, const FString& Message) { }
    static FORCEINLINE void RequestExit(int32 ExitCode) { }
    static FORCEINLINE void PumpMessages(bool bUntilEmpty) { }
    static FORCEINLINE FModifierKeyState GetModifierKeyState() { return FModifierKeyState(); }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
