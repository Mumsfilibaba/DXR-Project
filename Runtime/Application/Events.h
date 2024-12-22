#pragma once
#include "Application/Input/Keys.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericApplication.h"

class FEventResponse
{
public:
    static FEventResponse Handled()
    {
        return FEventResponse(true);
    }

    static FEventResponse Unhandled()
    {
        return FEventResponse(false);
    }

    bool IsEventHandled() const
    {
        return bIsHandled;
    }

private:
    FEventResponse(bool bInIsHandled)
        : bIsHandled(bInIsHandled)
    {
    }

    uint8 bIsHandled : 1;
};

enum class EInputEventType : uint8
{
    Unknown = 0,
    KeyDown,
    KeyUp,
    KeyChar,
    MouseMoved,
    MouseButtonDown,
    MouseButtonUp,
    MouseButtonDoubleClick,
    MouseScrolled,
    MouseEntered,
    MouseLeft,
    HighPrecisionMouse,
    GamepadButtonDown,
    GamepadButtonUp,
    GamepadAnalogSourceChanged,
};

class FInputEvent
{
public:
    FInputEvent() 
        : EventType(EInputEventType::Unknown)
        , ModifierKeys()
    {
    }

    FInputEvent(EInputEventType InEventType, const FModifierKeyState& InModifierKeys)
        : EventType(InEventType)
        , ModifierKeys(InModifierKeys)
    {
    }

    EInputEventType GetEventType() const
    {
        return EventType;
    }

    const FModifierKeyState& GetModifierKeys() const
    {
        return ModifierKeys;
    }
    
private:
    EInputEventType   EventType;
    FModifierKeyState ModifierKeys;
};

class FCursorEvent : public FInputEvent
{
public:
    FCursorEvent()
        : FInputEvent()
        , Key(EKeys::Unknown)
        , CursorPosition()
        , ScrollDelta(0.0f)
        , bIsScrollVertical(false)
        , bIsDown(false)
    {
    }

    FCursorEvent(EInputEventType InEventType, const FModifierKeyState& InModifierKeys)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(EKeys::Unknown)
        , CursorPosition()
        , ScrollDelta(0.0f)
        , bIsScrollVertical(false)
        , bIsDown(false)
    {
    }

    FCursorEvent(EInputEventType InEventType, const FIntVector2& InCursorPosition, const FModifierKeyState& InModifierKeys)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(EKeys::Unknown)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , bIsScrollVertical(false)
        , bIsDown(false)
    {
    }

    FCursorEvent(EInputEventType InEventType, FKey InKey, const FModifierKeyState& InModifierKeys)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(InKey)
        , CursorPosition()
        , ScrollDelta(0.0f)
        , bIsScrollVertical(false)
        , bIsDown(false)
    {
        CHECK(InKey.IsMouseButton());
    }

    FCursorEvent(EInputEventType InEventType, FKey InKey, const FModifierKeyState& InModifierKeys, bool bInIsDown)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(InKey)
        , CursorPosition()
        , ScrollDelta(0.0f)
        , bIsScrollVertical(false)
        , bIsDown(bInIsDown)
    {
        CHECK(InKey.IsMouseButton());
    }

    FCursorEvent(EInputEventType InEventType, const FModifierKeyState& InModifierKeys, float InScrollDelta, bool bInIsScrollVertical)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(EKeys::Unknown)
        , CursorPosition()
        , ScrollDelta(InScrollDelta)
        , bIsScrollVertical(bInIsScrollVertical)
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

    bool IsScrollVertical() const
    {
        return bIsScrollVertical;
    }

    bool IsDown() const
    {
        return bIsDown;
    }

    float GetScrollDelta() const
    {
        return ScrollDelta;
    }

private:
    FKey        Key;
    FIntVector2 CursorPosition;
    float       ScrollDelta;
    
    bool bIsScrollVertical : 1;
    bool bIsDown           : 1;
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

    FKeyEvent(EInputEventType InEventType, FKey InKey, const FModifierKeyState& InModifierKeys, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(InKey)
        , Character(0)
        , GamepadIndex(static_cast<uint32>(-1))
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    FKeyEvent(EInputEventType InEventType, FKey InKey, const FModifierKeyState& InModifierKeys, uint32 InCharacter, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(InKey)
        , Character(InCharacter)
        , GamepadIndex(static_cast<uint32>(-1))
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    FKeyEvent(EInputEventType InEventType, FKey InKey, const FModifierKeyState& InModifierKeys, uint32 InCharacter, uint32 InGamepadIndex, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(InEventType, InModifierKeys)
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

    FAnalogGamepadEvent(EInputEventType InEventType, EAnalogSourceName::Type InAnalogSource, uint32 InGamepadIndex, const FModifierKeyState& InModifierKeys, float InAnalogValue)
        : FInputEvent(InEventType, InModifierKeys)
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
