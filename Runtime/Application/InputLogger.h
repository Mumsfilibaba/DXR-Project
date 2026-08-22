#pragma once
#include "Application/InputHandler.h"

#ifndef APPLICATION_ENABLE_INPUT_LOGGING
    #define APPLICATION_ENABLE_INPUT_LOGGING (!RELEASE_BUILD)
#endif

#if APPLICATION_ENABLE_INPUT_LOGGING

class FInputLogger final : public FInputHandler
{
public:
    /** @brief Whether Input.EnableLogging asked for the logger. */
    static bool IsEnabled();

private:
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent) override final;
    virtual bool OnKeyChar(const FKeyEvent& KeyTypedEvent) override final;

    virtual bool OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual bool OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
    virtual bool OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;
    virtual bool OnMouseScrolled(const FCursorEvent& CursorEvent) override final;
    virtual bool OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) override final;

    virtual bool OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogEvent) override final;
};

#endif
