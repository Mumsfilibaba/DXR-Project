#pragma once
#include "Keys.h"
#include "CoreApplication/Generic/InputCodes.h"

class APPLICATION_API FInputMapper
{
public:
    static FInputMapper& Get() { return Instance; }

    void Initialize();

    FKey GetKeyboardKey(EKeyboardKeyName::Type Key);
    FKey GetMouseKey(EMouseButtonName::Type MouseButton);
    FKey GetGamepadKey(EGamepadButtonName::Type GamepadButton);

    EKeyboardKeyName::Type GetKeyboardKeyNameFromKey(FKey Key);
    EMouseButtonName::Type GetMouseButtonNameFromKey(FKey Key);
    EGamepadButtonName::Type GetGamepadButtonNameFromKey(FKey Key);

private:
    FKey KeyboardMap[EKeyboardKeyName::Count];
    FKey MouseMap[EMouseButtonName::Count];
    FKey GamepadMap[EGamepadButtonName::Count];

    static FInputMapper Instance;
};