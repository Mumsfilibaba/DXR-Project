#pragma once 
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"

#ifdef MessageBox
    #undef MessageBox
#endif

enum class EModifierFlag : uint32
{
    None     = 0,
    Ctrl     = FLAG(1),
    Alt      = FLAG(2),
    Shift    = FLAG(3),
    CapsLock = FLAG(4),
    Super    = FLAG(5),
    NumLock  = FLAG(6),
};

ENUM_CLASS_OPERATORS(EModifierFlag);

class FModifierKeyState
{
public:
    FModifierKeyState()
        : Flags(EModifierFlag::None)
    {
    }

    FModifierKeyState(EModifierFlag InFlags)
        : Flags(InFlags)
    {
    }

    bool IsCtrlDown() const
    {
        return (Flags & EModifierFlag::Ctrl) != EModifierFlag::None;
    }
    
    bool IsAltDown() const
    {
        return (Flags & EModifierFlag::Alt) != EModifierFlag::None;
    }
    
    bool IsShiftDown() const
    {
        return (Flags & EModifierFlag::Shift) != EModifierFlag::None;
    }
    
    bool IsCapsLockDown() const
    {
        return (Flags & EModifierFlag::CapsLock) != EModifierFlag::None;
    }
    
    bool IsSuperDown() const
    {
        return (Flags & EModifierFlag::Super) != EModifierFlag::None;
    }
    
    bool IsNumPadDown() const
    {
        return (Flags & EModifierFlag::NumLock) != EModifierFlag::None;
    }
    
private:
    EModifierFlag Flags;
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
