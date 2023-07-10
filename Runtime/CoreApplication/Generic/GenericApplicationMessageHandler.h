#pragma once
#include "Core/Input/InputCodes.h"
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

struct FGenericApplicationMessageHandler
{
    virtual ~FGenericApplicationMessageHandler() = default;

    virtual bool OnControllerButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex)
    {
        return false;
    }

    virtual bool OnControllerButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
    {
        return false;
    }

    virtual bool OnControllerAnalog(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
    {
        return false;
    }

    virtual bool OnKeyUp(EKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
    {
        return false;
    }

    virtual bool OnKeyDown(EKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
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

    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnMouseButtonDoubleClick(const TSharedRef<FGenericWindow>& Window, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnHighPrecisionMouseInput(const TSharedRef<FGenericWindow>& Window, int32 x, uint32 y)
    {
        return false;
    }

    virtual bool OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height)
    {
        return false;
    }

    virtual bool OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnWindowMouseEntered(const TSharedRef<FGenericWindow>& Window)
    {
        return false;
    }

    virtual bool OnWindowMouseLeft(const TSharedRef<FGenericWindow>& Window)
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

    virtual bool OnMonitorChange()
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
