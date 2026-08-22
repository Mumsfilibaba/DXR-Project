#pragma once
#include "Application/Input/Keys.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/PlatformInterface/InputCodes.h"
#include "CoreApplication/PlatformInterface/IPlatformApplication.h"

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

public:
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
        , Key(Keys::Unknown)
        , ScreenPosition()
        , ClientOrigin()
        , ScrollDelta(0.0f)
        , ScrollAxis(EScrollAxis::Vertical)
        , bIsDown(false)
    {
    }

    FCursorEvent(EInputEventType InEventType, const FModifierKeyState& InModifierKeys)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(Keys::Unknown)
        , ScreenPosition()
        , ClientOrigin()
        , ScrollDelta(0.0f)
        , ScrollAxis(EScrollAxis::Vertical)
        , bIsDown(false)
    {
    }

    FCursorEvent(EInputEventType InEventType, const IntVector2& InScreenPosition, const IntVector2& InClientOrigin, const FModifierKeyState& InModifierKeys)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(Keys::Unknown)
        , ScreenPosition(InScreenPosition)
        , ClientOrigin(InClientOrigin)
        , ScrollDelta(0.0f)
        , ScrollAxis(EScrollAxis::Vertical)
        , bIsDown(false)
    {
    }

    FCursorEvent(EInputEventType InEventType, FKey InKey, const FModifierKeyState& InModifierKeys)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(InKey)
        , ScreenPosition()
        , ClientOrigin()
        , ScrollDelta(0.0f)
        , ScrollAxis(EScrollAxis::Vertical)
        , bIsDown(false)
    {
        CHECK(InKey.IsMouseButton());
    }

    FCursorEvent(EInputEventType InEventType, FKey InKey, const FModifierKeyState& InModifierKeys, bool bInIsDown)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(InKey)
        , ScreenPosition()
        , ClientOrigin()
        , ScrollDelta(0.0f)
        , ScrollAxis(EScrollAxis::Vertical)
        , bIsDown(bInIsDown)
    {
        CHECK(InKey.IsMouseButton());
    }

    FCursorEvent(EInputEventType InEventType, FKey InKey, const IntVector2& InScreenPosition, const IntVector2& InClientOrigin, const FModifierKeyState& InModifierKeys, bool bInIsDown)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(InKey)
        , ScreenPosition(InScreenPosition)
        , ClientOrigin(InClientOrigin)
        , ScrollDelta(0.0f)
        , ScrollAxis(EScrollAxis::Vertical)
        , bIsDown(bInIsDown)
    {
        CHECK(InKey.IsMouseButton());
    }

    FCursorEvent(EInputEventType InEventType, const FModifierKeyState& InModifierKeys, float InScrollDelta, EScrollAxis InScrollAxis)
        : FInputEvent(InEventType, InModifierKeys)
        , Key(Keys::Unknown)
        , ScreenPosition()
        , ClientOrigin()
        , ScrollDelta(InScrollDelta)
        , ScrollAxis(InScrollAxis)
        , bIsDown(false)
    {
    }

    FKey GetKey() const
    {
        return Key;
    }

    /** @brief The position in screen space, which is the space the platform delivers the event in. */
    NODISCARD const IntVector2& GetScreenPosition() const
    {
        return ScreenPosition;
    }

    /** @brief The position in the space the elements were arranged in, which is the client area of the window under the cursor. */
    NODISCARD IntVector2 GetClientPosition() const
    {
        return ScreenPosition - ClientOrigin;
    }

    /** @brief The raw delta a high-precision event carries, which is a movement rather than a position. */
    NODISCARD const IntVector2& GetHighPrecisionDelta() const
    {
        CHECK(GetEventType() == EInputEventType::HighPrecisionMouse);
        return ScreenPosition;
    }

    EScrollAxis GetScrollAxis() const
    {
        return ScrollAxis;
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
    IntVector2  ScreenPosition;
    IntVector2  ClientOrigin;
    float       ScrollDelta;
    EScrollAxis ScrollAxis;
    bool        bIsDown : 1;
};

class FKeyEvent : public FInputEvent
{
public:
    FKeyEvent()
        : FInputEvent()
        , Key(Keys::Unknown)
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
