#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Input/InputCodes.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class PlatformCallbacks;
class GenericWindow;
class GenericCursor;

struct ModifierKeyState
{
public:
    ModifierKeyState() = default;

    FORCEINLINE ModifierKeyState( uint32 InModifierMask )
        : ModifierMask( InModifierMask )
    {
    }

    FORCEINLINE bool IsCtrlDown() const
    {
        return (ModifierMask & ModifierFlag_Ctrl);
    }

    FORCEINLINE bool IsAltDown() const
    {
        return (ModifierMask & ModifierFlag_Alt);
    }

    FORCEINLINE bool IsShiftDown() const
    {
        return (ModifierMask & ModifierFlag_Shift);
    }

    FORCEINLINE bool IsCapsLockDown() const
    {
        return (ModifierMask & ModifierFlag_CapsLock);
    }

    FORCEINLINE bool IsSuperKeyDown() const
    {
        return (ModifierMask & ModifierFlag_Super);
    }

    FORCEINLINE bool IsNumPadDown() const
    {
        return (ModifierMask & ModifierFlag_NumLock);
    }

    uint32 ModifierMask = 0;
};

class GenericPlatform
{
public:
    static bool Init()
    {
        return false;
    }

    static void Tick()
    {
    }

    static bool Release()
    {
        return false;
    }

    static ModifierKeyState GetModifierKeyState()
    {
        return ModifierKeyState();
    }

    static void SetCapture( GenericWindow* Window )
    {
    }
    static void SetActiveWindow( GenericWindow* Window )
    {
    }

    static GenericWindow* GetCapture()
    {
        return nullptr;
    }
    static GenericWindow* GetActiveWindow()
    {
        return nullptr;
    }

    static void SetCursor( GenericCursor* Cursor )
    {
    }
    static GenericCursor* GetCursor()
    {
        return nullptr;
    }

    static void SetCursorPos( GenericWindow* RelativeWindow, int32 x, int32 y )
    {
    }
    static void GetCursorPos( GenericWindow* RelativeWindow, int32& OutX, int32& OutY )
    {
    }

    static void SetCallbacks( PlatformCallbacks* InCallbacks );
    static PlatformCallbacks* GetCallbacks();

protected:
    static PlatformCallbacks* Callbacks;
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif