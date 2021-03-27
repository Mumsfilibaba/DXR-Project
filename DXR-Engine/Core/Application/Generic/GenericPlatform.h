#pragma once
#include "Core/Ref.h"
#include "Core/Input/InputCodes.h"

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class PlatformCallbacks;
class Window;
class Cursor;

struct ModifierKeyState
{
public:
    ModifierKeyState() = default;

    ModifierKeyState(uint32 InModifierMask)
        : ModifierMask(InModifierMask)
    {
    }

    bool IsCtrlDown() const { return (ModifierMask & ModifierFlag_Ctrl); }
    bool IsAltDown() const { return (ModifierMask & ModifierFlag_Alt); }
    bool IsShiftDown() const { return (ModifierMask & ModifierFlag_Shift); }
    bool IsCapsLockDown() const { return (ModifierMask & ModifierFlag_CapsLock); }
    bool IsSuperKeyDown() const { return (ModifierMask & ModifierFlag_Super); }
    bool IsNumPadDown() const { return (ModifierMask & ModifierFlag_NumLock); }

    uint32 ModifierMask = 0;
};

class GenericPlatform
{
public:
    static bool Init() { return false; }

    static void Tick() {}

    static bool Release() { return false; }

    static ModifierKeyState GetModifierKeyState() { return ModifierKeyState(); }

    static void SetCapture(Window* Window) {}
    static void SetActiveWindow(Window* Window) {}

    static Window* GetCapture() { return nullptr; }
    static Window* GetActiveWindow() { return nullptr; }

    static void SetCursor(Cursor* Cursor) {}
    static Cursor* GetCursor() { return nullptr; }

    static void SetCursorPos(Window* RelativeWindow, int32 x, int32 y) {}
    static void GetCursorPos(Window* RelativeWindow, int32& OutX, int32& OutY) {}

    static void SetCallbacks(PlatformCallbacks* InCallbacks);
    static PlatformCallbacks* GetCallbacks();

protected:
    static PlatformCallbacks* Callbacks;
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif