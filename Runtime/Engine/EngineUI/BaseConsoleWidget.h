#pragma once
#include "Application/InputHandler.h"

struct FConsoleInputHandler final : public FInputHandler
{
    DECLARE_DELEGATE(FHandleKeyEventDelegate, const FKeyEvent&);
    FHandleKeyEventDelegate HandleKeyEventDelegate;

    virtual bool OnKeyUp(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.ExecuteIfBound(KeyEvent);
        return bConsoleToggled;
    }

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.ExecuteIfBound(KeyEvent);
        return bConsoleToggled;
    }

    bool bConsoleToggled = false;
};
