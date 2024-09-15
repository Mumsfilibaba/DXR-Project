#pragma once
#include "Application/Input/Keys.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericWindow.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

class FResponse
{
    FResponse(bool bInIsHandled)
        : bIsHandled(bInIsHandled)
    {
    }

public:
    static FResponse Handled()
    {
        return FResponse(true);
    }

    static FResponse Unhandled()
    {
        return FResponse(false);
    }

    bool IsEventHandled() const
    {
        return bIsHandled;
    }

private:
    uint32 bIsHandled : 1;
};

class FInputEvent
{
public:
    FInputEvent() 
        : ModifierKeys()
    {
    }

    FInputEvent(const FModifierKeyState& InModifierKeys)
        : ModifierKeys(InModifierKeys)
    {
    }

    const FModifierKeyState& GetModifierKeys() const
    {
        return ModifierKeys;
    }

private:
    FModifierKeyState ModifierKeys;
};

enum class EButtomEventType
{
    Unknown = 0,
    Pressed,
    Released,
    DoubleClick
};

class FCursorEvent : public FInputEvent
{
public:
    FCursorEvent()
        : FInputEvent()
        , Key(EKeys::Unknown)
        , CursorPosition()
        , ScrollDelta(0.0f)
        , bIsVerticalScrollDelta(false)
        , bIsDown(false)
    {
    }

    FCursorEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& InModifierKeys)
        : FInputEvent(InModifierKeys)
        , Key(EKeys::Unknown)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , bIsVerticalScrollDelta(false)
        , bIsDown(false)
    {
    }

    FCursorEvent(FKey InKey, const FIntVector2& InCursorPosition, const FModifierKeyState& InModifierKeys, bool bInIsDown)
        : FInputEvent(InModifierKeys)
        , Key(InKey)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , bIsVerticalScrollDelta(false)
        , bIsDown(bInIsDown)
    {
    }

    FCursorEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& InModifierKeys, float InScrollDelta, bool bInIsVerticalScrollDelta)
        : FInputEvent(InModifierKeys)
        , Key(EKeys::Unknown)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(InScrollDelta)
        , bIsVerticalScrollDelta(bInIsVerticalScrollDelta)
        , bIsDown(false)
    {
    }

    FKey GetKey() const
    {
        return Key;
    }

    const FIntVector2& GetCursorPos() const
    {
        return CursorPosition;
    }

    float GetScrollDelta() const
    {
        return ScrollDelta;
    }

    bool IsVerticalScrollDelta() const
    {
        return bIsVerticalScrollDelta;
    }

    bool IsDown() const
    {
        return bIsDown;
    }

private:
    FKey        Key;
    FIntVector2 CursorPosition;
    float       ScrollDelta;
    bool        bIsVerticalScrollDelta : 1;
    bool        bIsDown                : 1;
};

class FKeyEvent : public FInputEvent
{
public:
    FKeyEvent()
        : FInputEvent()
        , Key(EKeys::Unknown)
        , Character(0)
        , GamepadIndex(static_cast<uint32>(-1))
        , bIsRepeat(false)
        , bIsDown(false)
    {
    }

    FKeyEvent(FKey InKey, const FModifierKeyState& InModifierKeys, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(InModifierKeys)
        , Key(InKey)
        , Character(0)
        , GamepadIndex(static_cast<uint32>(-1))
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    FKeyEvent(FKey InKey, const FModifierKeyState& InModifierKeys, uint32 InCharacter, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(InModifierKeys)
        , Key(InKey)
        , Character(InCharacter)
        , GamepadIndex(static_cast<uint32>(-1))
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    FKeyEvent(FKey InKey, const FModifierKeyState& InModifierKeys, uint32 InCharacter, uint32 InGamepadIndex, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(InModifierKeys)
        , Key(InKey)
        , Character(InCharacter)
        , GamepadIndex(InGamepadIndex)
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    FKey GetKey() const
    {
        return Key;
    }

    CHAR GetAnsiChar() const
    {
        return static_cast<CHAR>(Character);
    }

    uint32 GetGamepadIndex() const
    {
        return GamepadIndex;
    }

    bool IsRepeat() const
    {
        return bIsRepeat;
    }

    bool IsDown() const
    {
        return bIsDown;
    }

private:
    FKey   Key;
    uint32 Character;
    uint32 GamepadIndex : 30;
    uint32 bIsRepeat    : 1;
    uint32 bIsDown      : 1;
};

class FAnalogGamepadEvent : public FInputEvent
{
public:
    FAnalogGamepadEvent()
        : FInputEvent()
        , AnalogSource(EAnalogSourceName::Unknown)
        , AnalogValue(0.0f)
        , GamepadIndex(0)
    {
    }

    FAnalogGamepadEvent(EAnalogSourceName::Type InAnalogSource, uint32 InGamepadIndex, const FModifierKeyState& InModifierKeys, float InAnalogValue)
        : FInputEvent(InModifierKeys)
        , AnalogSource(InAnalogSource)
        , AnalogValue(InAnalogValue)
        , GamepadIndex(InGamepadIndex)
    {
    }

    EAnalogSourceName::Type GetAnalogSource() const
    {
        return AnalogSource;
    }

    uint32 GetControllerIndex() const
    {
        return GamepadIndex;
    }

    float GetAnalogValue() const
    {
        return AnalogValue;
    }

private:
    EAnalogSourceName::Type AnalogSource;
    float                   AnalogValue;
    uint32                  GamepadIndex;
};
