#pragma once
#include "GenericWindow.h"
#include "GenericCursor.h"

#include "Application/InputCodes.h"

#include "GenericApplicationEventHandler.h"

#include "Core/Ref.h"

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

class GenericApplication
{
public:
    virtual ~GenericApplication() = default;

    virtual GenericWindow* MakeWindow() = 0;
    virtual GenericCursor* MakeCursor() = 0;

    virtual bool Init() = 0;

    /*
    * Events gets stored and is processed in this function. This is because events sometimes are sent
    * from different functions than PeekMessageUntilNoMessage, For example GenericWindow::ToggleFullscreen. This
    * makes sure that all events are processed at one time.
    */

    virtual void Tick() = 0;

    virtual void SetCursor(GenericCursor* Cursor) = 0;
    virtual GenericCursor* GetCursor() const = 0;

    virtual void SetActiveWindow(GenericWindow* Window) = 0;
    
    virtual void SetCapture(GenericWindow* Window)
    {
        UNREFERENCED_VARIABLE(Window);
    }

    virtual GenericWindow* GetActiveWindow() const = 0;

    // Some platforms does not have the concept of mouse capture, therefor return nullptr as standard
    virtual GenericWindow* GetCapture() const { return nullptr; }

    virtual void SetCursorPos(GenericWindow* RelativeWindow, int32 x, int32 y) = 0;
    virtual void GetCursorPos(GenericWindow* RelativeWindow, int32& OutX, int32& OutY) const = 0;

    void SetEventHandler(GenericApplicationEventHandler* InEventHandler)
    {
        EventHandler = InEventHandler;
    }

    GenericApplicationEventHandler* GetEventHandler() const
    {
        return EventHandler;
    }

    static bool PeekMessageUntilNoMessage()
    {
        return false;
    }

    static ModifierKeyState GetModifierKeyState()
    {
        return ModifierKeyState();
    }

    static GenericApplication* Make()
    {
        return nullptr;
    }

protected:
    GenericApplicationEventHandler* EventHandler;
};