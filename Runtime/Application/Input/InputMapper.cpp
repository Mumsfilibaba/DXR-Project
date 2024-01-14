#include "InputMapper.h"

FInputMapper FInputMapper::Instance;

void FInputMapper::Initialize()
{
    // Init the KeyboardKey map
    static_assert(ToUnderlying(EKeyboardKeyName::Unknown) == ToUnderlying(EKeyName::Unknown) && ToUnderlying(EKeyboardKeyName::Last) == ToUnderlying(EKeyName::Menu), "EKeyboardKeyName::Type has changed values, update mapping");

    for (int32 Index = EKeyboardKeyName::Unknown; Index <= EKeyboardKeyName::Last; Index++)
    {
        const EKeyName::Type KeyName = static_cast<EKeyName::Type>(Index);
        KeyboardMap[Index] = FKey(KeyName);
    }

    // Init the MouseButton map
    static_assert(EMouseButtonName::Left == 1 && EMouseButtonName::Last == 5, "EMouseButtonName::Type has changed values, update mapping");

    MouseMap[0] = EKeys::Unknown;
    for (int32 Index = EMouseButtonName::Left; Index <= EMouseButtonName::Last; Index++)
    {
        const EKeyName::Type MouseButtonName = static_cast<EKeyName::Type>(EKeyName::MouseButtonLeft + Index - 1);
        MouseMap[Index] = FKey(MouseButtonName);
    }

    // Init the MouseButton map
    static_assert(EGamepadButtonName::DPadUp == 1 && EGamepadButtonName::Last == 14, "EGamepadButtonName::Type has changed values, update mapping");

    GamepadMap[0] = EKeys::Unknown;
    for (int32 Index = EGamepadButtonName::DPadUp; Index <= EGamepadButtonName::Last; Index++)
    {
        const EKeyName::Type GamepadButtonName = static_cast<EKeyName::Type>(EKeyName::GamepadDPadUp + Index - 1);
        GamepadMap[Index] = FKey(GamepadButtonName);
    }
}

FKey FInputMapper::GetKeyboardKey(EKeyboardKeyName::Type Key)
{
    return KeyboardMap[Key];
}

FKey FInputMapper::GetMouseKey(EMouseButtonName::Type MouseButton)
{
    return MouseMap[MouseButton];
}

FKey FInputMapper::GetGamepadKey(EGamepadButtonName::Type GamepadButton)
{
    return GamepadMap[GamepadButton];
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
