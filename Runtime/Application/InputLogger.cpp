#include "Application/InputLogger.h"
#include "Core/Containers/String.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"

#if APPLICATION_ENABLE_INPUT_LOGGING

static TAutoConsoleVariable<bool> CVarEnableInputLogging(
    "Input.EnableLogging",
    "Registers the handler that logs input events, read once as the application starts",
    false);

static TAutoConsoleVariable<bool> CVarLogKeyEvents(
    "Input.LogKeyEvents",
    "Logs every key and every character the platform delivers, with the modifiers each arrived under",
    false);

static TAutoConsoleVariable<bool> CVarLogMouseEvents(
    "Input.LogMouseEvents",
    "Logs every mouse event the platform delivers, moves included, which arrive often enough to flood the log",
    false);

static TAutoConsoleVariable<bool> CVarLogGamepadEvents(
    "Input.LogGamepadEvents",
    "Logs every analog change the platform delivers, which arrive for as long as a stick is off centre",
    false);

static String DescribeModifiers(const FModifierKeyState& Modifiers)
{
    String Description;
    if (Modifiers.IsCtrlDown())
    {
        Description += "Ctrl ";
    }
    if (Modifiers.IsAltDown())
    {
        Description += "Alt ";
    }
    if (Modifiers.IsShiftDown())
    {
        Description += "Shift ";
    }
    if (Modifiers.IsSuperDown())
    {
        Description += "Super ";
    }

    return Description.IsEmpty() ? String("none") : Description;
}

bool FInputLogger::IsEnabled()
{
    return CVarEnableInputLogging.GetValue();
}

bool FInputLogger::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (CVarLogKeyEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: KeyDown '%s'%s, modifiers %s", KeyEvent.GetKey().ToString(), KeyEvent.IsRepeat() ? " (repeat)" : "", *DescribeModifiers(KeyEvent.GetModifierKeys()));
    }

    return false;
}

bool FInputLogger::OnKeyUp(const FKeyEvent& KeyEvent)
{
    if (CVarLogKeyEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: KeyUp '%s', modifiers %s", KeyEvent.GetKey().ToString(), *DescribeModifiers(KeyEvent.GetModifierKeys()));
    }

    return false;
}

bool FInputLogger::OnKeyChar(const FKeyEvent& KeyTypedEvent)
{
    if (CVarLogKeyEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: KeyChar 0x%02x '%c', modifiers %s", KeyTypedEvent.GetAnsiChar(), KeyTypedEvent.GetAnsiChar(), *DescribeModifiers(KeyTypedEvent.GetModifierKeys()));
    }

    return false;
}

bool FInputLogger::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (CVarLogMouseEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: MouseMove to screen (%d, %d), client (%d, %d)", CursorEvent.GetScreenPosition().X, CursorEvent.GetScreenPosition().Y, CursorEvent.GetClientPosition().X, CursorEvent.GetClientPosition().Y);
    }

    return false;
}

bool FInputLogger::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CVarLogMouseEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: MouseButtonDown '%s' at client (%d, %d), modifiers %s", CursorEvent.GetKey().ToString(), CursorEvent.GetClientPosition().X, CursorEvent.GetClientPosition().Y, *DescribeModifiers(CursorEvent.GetModifierKeys()));
    }

    return false;
}

bool FInputLogger::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (CVarLogMouseEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: MouseButtonUp '%s' at client (%d, %d), modifiers %s", CursorEvent.GetKey().ToString(), CursorEvent.GetClientPosition().X, CursorEvent.GetClientPosition().Y, *DescribeModifiers(CursorEvent.GetModifierKeys()));
    }

    return false;
}

bool FInputLogger::OnMouseScrolled(const FCursorEvent& CursorEvent)
{
    if (CVarLogMouseEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: MouseScrolled %.4f on the %s axis, modifiers %s", CursorEvent.GetScrollDelta(), ToString(CursorEvent.GetScrollAxis()), *DescribeModifiers(CursorEvent.GetModifierKeys()));
    }

    return false;
}

bool FInputLogger::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    if (CVarLogMouseEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: HighPrecisionMouse delta (%d, %d)", CursorEvent.GetHighPrecisionDelta().X, CursorEvent.GetHighPrecisionDelta().Y);
    }

    return false;
}

bool FInputLogger::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogEvent)
{
    if (CVarLogGamepadEvents.GetValue())
    {
        LOG_INFO("[FInputLogger]: AnalogGamepadChange '%s' = %.4f on gamepad %u", ToString(AnalogEvent.GetAnalogSource()), AnalogEvent.GetAnalogValue(), AnalogEvent.GetControllerIndex());
    }

    return false;
}

#endif
