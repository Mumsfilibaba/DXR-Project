#include "InputMapper.h"

FInputMapper FInputMapper::Instance;

FKey FInputMapper::GetKeyboardKey(EKeyboardKeyName::Type Key)
{
    const EKeyName::Type KeyName = static_cast<EKeyName::Type>(Key + EKeyName::Unknown);
    return FKey(KeyName);
}

FKey FInputMapper::GetMouseKey(EMouseButtonName::Type MouseButton)
{
    const EKeyName::Type KeyName = static_cast<EKeyName::Type>(MouseButton + EKeyName::MouseButtonLeft);
    return FKey(KeyName);
}

FKey FInputMapper::GetGamepadKey(EGamepadButtonName::Type GamepadButton)
{
    const EKeyName::Type KeyName = static_cast<EKeyName::Type>(GamepadButton + EKeyName::GamepadDPadUp);
    return FKey(KeyName);
}

EKeyboardKeyName::Type FInputMapper::GetKeyboardKeyNameFromKey(FKey Key)
{
    const EKeyboardKeyName::Type KeyboardKeyName = static_cast<EKeyboardKeyName::Type>(Key.GetKeyName());
    return KeyboardKeyName;
}

EMouseButtonName::Type FInputMapper::GetMouseButtonNameFromKey(FKey Key)
{
    const EMouseButtonName::Type MouseButtonName = static_cast<EMouseButtonName::Type>(Key.GetKeyName() - EKeyName::MouseButtonLeft);
    return MouseButtonName;
}

EGamepadButtonName::Type FInputMapper::GetGamepadButtonNameFromKey(FKey Key)
{
    const EGamepadButtonName::Type GamepadButtonName = static_cast<EGamepadButtonName::Type>(Key.GetKeyName() - EKeyName::GamepadDPadUp);
    return GamepadButtonName;
}