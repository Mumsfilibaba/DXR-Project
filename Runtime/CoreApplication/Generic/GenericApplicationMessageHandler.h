#pragma once
#include "GenericWindow.h"
#include "Core/Input/InputCodes.h"
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericApplicationMessageHandler
{
    virtual ~FGenericApplicationMessageHandler() = default;

    virtual bool OnControllerButtonUp(EControllerButton Button, uint32 ControllerIndex)
    {
        return false;
    }

    virtual bool OnControllerButtonDown(EControllerButton Button, uint32 ControllerIndex)
    {
        return false;
    }

    virtual bool OnControllerAnalog(EControllerAnalog AnalogSource, uint32 ControllerIndex, float AnalogValue)
    {
        return false;
    }

    virtual bool OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState)
    {
        return false;
    }

    virtual bool OnKeyDown(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
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

    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnMouseButtonDoubleClick(const TSharedRef<FGenericWindow>& Window, EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
    {
        return false;
    }

    virtual bool OnMouseButtonUp(EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
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

    virtual bool OnApplicationExit(int32 ExitCode)
    {
        return false;
    }

    virtual bool OnMonitorChange()
    {
        return false;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
