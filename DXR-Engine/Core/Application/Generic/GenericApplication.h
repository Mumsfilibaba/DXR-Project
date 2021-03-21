#pragma once
#include "GenericWindow.h"
#include "GenericCursor.h"

#include "Core/Application/InputCodes.h"

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

    virtual GenericWindow* CreateWindow(const std::string& Title, uint32 Width, uint32 Height, WindowStyle Style) = 0;

    virtual bool Init() = 0;
    virtual void Tick() = 0;
    virtual void Release() = 0;

    virtual ModifierKeyState GetModifierKeyState() = 0;
    
    virtual void SetCapture(GenericWindow* Window) { UNREFERENCED_VARIABLE(Window); }
    virtual void SetActiveWindow(GenericWindow* Window) = 0;

    virtual GenericWindow* GetCapture() const { return nullptr; }
    virtual GenericWindow* GetActiveWindow() const = 0;

    void SetEventHandler(GenericApplicationEventHandler* InEventHandler) { EventHandler = InEventHandler; }
    GenericApplicationEventHandler* GetEventHandler() const { return EventHandler; }

    static GenericApplication& Get();

protected:
    GenericApplicationEventHandler* EventHandler;
};