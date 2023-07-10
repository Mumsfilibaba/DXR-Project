#pragma once
#include "Core/Core.h"
#include "Core/Input/ModifierKeyState.h"
#include "Core/Input/InputCodes.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/GenericWindow.h"

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


class FMouseEvent : public FInputEvent
{
public:
    FMouseEvent()
        : FInputEvent()
        , CursorPosition()
        , ScrollDelta(0.0f)
        , Button(EMouseButtonName::Unknown)
        , bIsVerticalScrollDelta(false)
        , bIsDown(false)
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys)
        : FInputEvent(ModifierKeys)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , Button(EMouseButtonName::Unknown)
        , bIsVerticalScrollDelta(false)
        , bIsDown(false)
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys, EMouseButtonName::Type InButton, bool bInIsDown)
        : FInputEvent(ModifierKeys)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , Button(InButton)
        , bIsVerticalScrollDelta(false)
        , bIsDown(bInIsDown)
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys, float InScrollDelta, bool bInIsVerticalScrollDelta)
        : FInputEvent(ModifierKeys)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(InScrollDelta)
        , Button(EMouseButtonName::Unknown)
        , bIsVerticalScrollDelta(bInIsVerticalScrollDelta)
        , bIsDown(false)
    {
    }

    FIntVector2 GetCursorPos() const
    {
        return CursorPosition;
    }

    EMouseButtonName::Type GetButton() const
    {
        return Button;
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
    FIntVector2  CursorPosition;
    float        ScrollDelta;
    EMouseButtonName::Type Button;
    bool         bIsVerticalScrollDelta : 1;
    bool         bIsDown : 1;
};


class FKeyEvent : public FInputEvent
{
public:
    FKeyEvent()
        : FInputEvent()
        , Character(0)
        , Key(EKeyName::Unknown)
        , bIsRepeat(false)
        , bIsDown(false)
    {
    }

    FKeyEvent(const FModifierKeyState& ModifierKeys, EKeyName::Type InKey, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(ModifierKeys)
        , Character(0)
        , Key(InKey)
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    FKeyEvent(const FModifierKeyState& ModifierKeys, EKeyName::Type InKey, uint32 InCharacter, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(ModifierKeys)
        , Character(InCharacter)
        , Key(InKey)
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    EKeyName::Type GetKey() const
    {
        return Key;
    }

    char GetAnsiChar() const
    {
        return static_cast<char>(Character);
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
    uint32 Character;
    EKeyName::Type Key;

    bool bIsRepeat : 1;
    bool bIsDown   : 1;
};


class FControllerEvent : public FInputEvent
{
public:
    FControllerEvent()
        : FInputEvent()
        , Button(EGamepadButtonName::Unknown)
        , bIsButtonDown(false)
        , bIsRepeat(false)
        , AnalogSource(EAnalogSourceName::Unknown)
        , AnalogValue(0.0f)
        , GamepadIndex(0)
    {
    }

    FControllerEvent(EGamepadButtonName::Type InButton, uint32 InControllerIndex, bool bInIsButtonDown, bool bInIsRepeat)
        : FInputEvent()
        , Button(InButton)
        , bIsButtonDown(bInIsButtonDown)
        , bIsRepeat(bInIsRepeat)
        , AnalogSource(EAnalogSourceName::Unknown)
        , AnalogValue(0.0f)
        , GamepadIndex(InControllerIndex)
    {
    }

    FControllerEvent(EAnalogSourceName::Type InAnalogSource, uint32 InControllerIndex, float InAnalogValue)
        : FInputEvent()
        , Button(EGamepadButtonName::Unknown)
        , bIsButtonDown(false)
        , bIsRepeat(false)
        , AnalogSource(InAnalogSource)
        , AnalogValue(InAnalogValue)
        , GamepadIndex(InControllerIndex)
    {
    }

    bool IsButtonDown() const
    {
        return bIsButtonDown;
    }

    bool IsRepeat() const
    {
        return bIsRepeat;
    }

    bool HasAnalogValue() const
    {
        return AnalogSource != EAnalogSourceName::Unknown;
    }

    EGamepadButtonName::Type GetButton() const
    {
        return Button;
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
    EGamepadButtonName::Type Button;
    bool               bIsButtonDown;
    bool               bIsRepeat;
    EAnalogSourceName::Type  AnalogSource;
    float              AnalogValue;
    uint32             GamepadIndex;
};

class FWindowEvent
{
public:
    FWindowEvent()
        : Window(nullptr)
        , Position()
        , Width(0)
        , Height(0)
    {
    }

    FWindowEvent(const TSharedRef<FGenericWindow>& InWindow)
        : Window(InWindow)
        , Position()
        , Width(0)
        , Height(0)
    {
    }

    FWindowEvent(const TSharedRef<FGenericWindow>& InWindow, const FIntVector2& InPosition)
        : Window(InWindow)
        , Position(InPosition)
        , Width(0)
        , Height(0)
    {
    }

    FWindowEvent(const TSharedRef<FGenericWindow>& InWindow, int32 InWidth, int32 InHeight)
        : Window(InWindow)
        , Position()
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    TSharedRef<FGenericWindow> GetWindow() const
    {
        return Window;
    }

    int32 GetWidth() const
    {
        return Width;
    }

    int32 GetHeight() const
    {
        return Height;
    }

    FIntVector2 GetPosition() const
    {
        return Position;
    }

private:
    TSharedRef<FGenericWindow> Window;
    FIntVector2 Position;
    int32       Width;
    int32       Height;
};
