#pragma once
#include "Keys.h"
#include "CoreApplication/Generic/InputCodes.h"

class FInputMapper
{
public:
    static FInputMapper& Get() { return Instance; }

    FKey GetKeyboardKey(EKeyboardKeyName::Type Key);

    FKey GetMouseKey(EMouseButtonName::Type MouseButton);

    FKey GetGamepadKey(EGamepadButtonName::Type GamepadButton);

    EKeyboardKeyName::Type GetKeyboardKeyNameFromKey(FKey Key);

    EMouseButtonName::Type GetMouseButtonNameFromKey(FKey Key);

    EGamepadButtonName::Type GetGamepadButtonNameFromKey(FKey Key);

private:
    static FInputMapper Instance;
};