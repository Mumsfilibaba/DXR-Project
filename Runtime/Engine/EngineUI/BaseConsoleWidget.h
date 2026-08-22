#pragma once
#include "Application/InputHandler.h"
#include "Application/Console/ConsoleElement.h"

enum class EConsoleKeyConsumption : uint8
{
    /**
     * Keeps every key while the console is toggled. The ImGui consoles need this, since their input
     * field is not an element and is therefore never on the focus path.
     */
    AllKeysWhileToggled,

    /**
     * Keeps only the key that opens and closes the console. A console built from elements needs this,
     * so that backspace, the arrows, Tab and Enter still reach its focused input.
     */
    ToggleKeyOnly,
};

class FConsoleInputHandler final : public FInputHandler
{
public:
    DECLARE_DELEGATE(FHandleKeyEventDelegate, const FKeyEvent&);

    virtual bool OnKeyUp(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.ExecuteIfBound(KeyEvent);
        return ShouldConsumeKey(KeyEvent);
    }

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final
    {
        HandleKeyEventDelegate.ExecuteIfBound(KeyEvent);

        const bool bConsumeKey = ShouldConsumeKey(KeyEvent);
        bSwallowNextChar = bConsumeKey && KeyConsumption == EConsoleKeyConsumption::ToggleKeyOnly;
        return bConsumeKey;
    }

    virtual bool OnKeyChar(const FKeyEvent& /*KeyEvent*/) override final
    {
        const bool bSwallowChar = bSwallowNextChar;
        bSwallowNextChar = false;
        return bSwallowChar;
    }

    FHandleKeyEventDelegate HandleKeyEventDelegate;
    EConsoleKeyConsumption  KeyConsumption  = EConsoleKeyConsumption::AllKeysWhileToggled;
    bool                    bConsoleToggled = false;

private:
    bool ShouldConsumeKey(const FKeyEvent& KeyEvent) const
    {
        if (KeyConsumption == EConsoleKeyConsumption::ToggleKeyOnly)
        {
            return FConsoleElement::IsToggleKey(KeyEvent.GetKey());
        }

        return bConsoleToggled;
    }

    bool bSwallowNextChar = false;
};
