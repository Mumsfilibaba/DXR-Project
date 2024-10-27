#pragma once
#include "GenericApplicationMisc.h"
#include "InputCodes.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

struct FGenericApplicationMessageHandler
{
    virtual ~FGenericApplicationMessageHandler() = default;

    virtual bool OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex)
    {
        return false;
    }

    virtual bool OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
    {
        return false;
    }

    virtual bool OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
    {
        return false;
    }

    virtual bool OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
    {
        return false;
    }

    virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
    {
        return false;
    }

    virtual bool OnKeyChar(uint32 Character)
    {
        return false;
    }

    virtual bool OnMouseMove(int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
    {
        return false;
    }

    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
    {
        return false;
    }

    virtual bool OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
    {
        return false;
    }

    virtual bool OnMouseScrolled(float WheelDelta, bool bVertical)
    {
        return false;
    }

    virtual bool OnMouseEntered()
    {
        return false;
    }

    virtual bool OnMouseLeft()
    {
        return false;
    }

    virtual bool OnHighPrecisionMouseInput(int32 x, uint32 y)
    {
        return false;
    }

    virtual bool OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height)
    {
        return false;
    }
    
    virtual bool OnWindowResizing(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    virtual bool OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    virtual bool OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    virtual bool OnWindowClosed(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    virtual bool OnMonitorConfigurationChange()
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
