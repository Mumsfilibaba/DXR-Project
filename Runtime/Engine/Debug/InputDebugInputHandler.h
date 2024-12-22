#pragma once
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/InputHandler.h"

struct FInputDebugInputHandler : public FInputHandler
{
    virtual bool OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogEvent)
    {
        LOG_INFO("OnAnalogGamepadChange '%s' = %.4f", ToString(AnalogEvent.GetAnalogSource()), AnalogEvent.GetAnalogValue());
        return false;
    }

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent)
    {
        LOG_INFO("OnKeyDown '%s' bIsRepeat=%s", KeyEvent.GetKey().ToString(), KeyEvent.IsRepeat() ? "true" : "false");
        return false;
    }

    virtual bool OnKeyUp(const FKeyEvent& KeyEvent)
    {
        LOG_INFO("OnKeyUp '%s'", KeyEvent.GetKey().ToString());
        return false;
    }

    virtual bool OnKeyChar(const FKeyEvent& KeyTypedEvent)
    {
        LOG_INFO("OnKeyChar '%c'", KeyTypedEvent.GetAnsiChar());
        return false;
    }

    virtual bool OnMouseMove(const FCursorEvent& CursorEvent)
    {
        LOG_INFO("OnMouseMove x: %d y: %d", CursorEvent.GetCursorPos().X, CursorEvent.GetCursorPos().Y);
        return false;
    }

    virtual bool OnMouseButtonDown(const FCursorEvent& CursorEvent)
    {
        LOG_INFO("OnMouseButtonDown '%s'", CursorEvent.GetKey().ToString());
        return false;
    }

    virtual bool OnMouseButtonUp(const FCursorEvent& CursorEvent)
    {
        LOG_INFO("OnMouseButtonUp '%s'", CursorEvent.GetKey().ToString());
        return false;
    }

    virtual bool OnMouseScrolled(const FCursorEvent& CursorEvent)
    {
        LOG_INFO("OnMouseScrolled '%.4f'", CursorEvent.GetScrollDelta());
        return false;
    }

    virtual bool OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
    {
        LOG_INFO("OnHighPrecisionMouseInput x: %d y: %d", CursorEvent.GetCursorPos().X, CursorEvent.GetCursorPos().Y);
        return false;
    }
};

