#pragma once
#include "GenericWindow.h"
#include "GenericCursor.h"

#include "Application/InputCodes.h"

#include "GenericApplicationDelegate.h"

#include "Core/TSharedRef.h"

struct ModifierKeyState
{
public:
    ModifierKeyState() = default;

    ModifierKeyState(UInt32 InModifierMask)
        : ModifierMask(InModifierMask)
    {
    }

    Bool IsCtrlDown() const { return (ModifierMask & MODIFIER_FLAG_CTRL); }
    Bool IsAltDown() const { return (ModifierMask & MODIFIER_FLAG_ALT); }
    Bool IsShiftDown() const { return (ModifierMask & MODIFIER_FLAG_SHIFT); }
    Bool IsCapsLockDown() const { return (ModifierMask & MODIFIER_FLAG_CAPS_LOCK); }
    Bool IsSuperKeyDown() const { return (ModifierMask & MODIFIER_FLAG_SUPER); }
    Bool IsNumPadDown() const { return (ModifierMask & MODIFIER_FLAG_NUM_LOCK); }

    UInt32 ModifierMask = 0;
};

class GenericApplication
{
public:
    virtual ~GenericApplication() = default;

    virtual GenericWindow* MakeWindow() = 0;
    virtual GenericCursor* MakeCursor() = 0;

    virtual Bool Init() = 0;

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

    virtual void SetCursorPos(GenericWindow* RelativeWindow, Int32 x, Int32 y) = 0;
    virtual void GetCursorPos(GenericWindow* RelativeWindow, Int32& OutX, Int32& OutY) const = 0;

    void SetEventHandler(GenericApplicationEventHandler* InEventHandler)
    {
        EventHandler = InEventHandler;
    }

    GenericApplicationEventHandler* GetEventHandler() const
    {
        return EventHandler;
    }

    static Bool PeekMessageUntilNoMessage()
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