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
        , Button(MouseButton_Unknown)
        , bIsVerticalScrollDelta(false)
        , bIsDown(false)
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys)
        : FInputEvent(ModifierKeys)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , Button(MouseButton_Unknown)
        , bIsVerticalScrollDelta(false)
        , bIsDown(false)
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys, EMouseButton InButton, bool bInIsDown)
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
        , Button(MouseButton_Unknown)
        , bIsVerticalScrollDelta(bInIsVerticalScrollDelta)
        , bIsDown(false)
    {
    }

    FIntVector2 GetCursorPos() const
    {
        return CursorPosition;
    }

    EMouseButton GetButton() const
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
    EMouseButton Button;
    bool         bIsVerticalScrollDelta : 1;
    bool         bIsDown : 1;
};


class FKeyEvent : public FInputEvent
{
public:
    FKeyEvent()
        : FInputEvent()
        , Key(Key_Unknown)
        , Character(0)
        , bIsRepeat(false)
    {
    }

    FKeyEvent(const FModifierKeyState& ModifierKeys, EKey InKey, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(ModifierKeys)
        , Key(InKey)
        , Character(0)
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    FKeyEvent(const FModifierKeyState& ModifierKeys, EKey InKey, uint32 InCharacter, bool bInIsRepeat, bool bInIsDown)
        : FInputEvent(ModifierKeys)
        , Key(InKey)
        , Character(InCharacter)
        , bIsRepeat(bInIsRepeat)
        , bIsDown(bInIsDown)
    {
    }

    EKey GetKey() const
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
    EKey   Key;

    bool bIsRepeat : 1;
    bool bIsDown   : 1;
};


class FControllerEvent : public FInputEvent
{
public:
    FControllerEvent()
        : FInputEvent()
        , Button(EControllerButton::Unknown)
        , ControllerIndex(0)
        , AnalogSource(EControllerAnalog::Unknown)
        , AnalogValue(0.0f)
    {
    }

    FControllerEvent(EControllerButton InButton, bool bInIsButtonDown, uint32 InControllerIndex)
        : FInputEvent()
        , Button(InButton)
        , bIsButtonDown(bInIsButtonDown)
        , ControllerIndex(ControllerIndex)
        , AnalogSource(EControllerAnalog::Unknown)
        , AnalogValue(0.0f)
    {
    }

    FControllerEvent(EControllerAnalog InAnalogSource, float InAnalogValue, uint32 InControllerIndex)
        : FInputEvent()
        , Button(EControllerButton::Unknown)
        , bIsButtonDown(false)
        , ControllerIndex(InControllerIndex)
        , AnalogSource(InAnalogSource)
        , AnalogValue(InAnalogValue)
    {
    }

    EControllerButton GetButton() const
    {
        return Button;
    }

    EControllerAnalog GetAnalogSource() const
    {
        return AnalogSource;
    }

    uint32 GetControllerIndex() const
    {
        return ControllerIndex;
    }

    float GetAnalogValue() const
    {
        return AnalogValue;
    }

    bool IsButtonDown() const
    {
        return bIsButtonDown;
    }

private:
    EControllerButton Button;
    bool              bIsButtonDown;
    EControllerAnalog AnalogSource;
    float             AnalogValue;
    uint32            ControllerIndex;
};

class FWindowEvent
{
public:
    FWindowEvent()
        : Window(nullptr)
        , Width(0)
        , Height(0)
        , Position()
    {
    }

    FWindowEvent(const TSharedRef<FGenericWindow>& InWindow)
        : Window(InWindow)
        , Width(0)
        , Height(0)
        , Position()
    {
    }

    FWindowEvent(const TSharedRef<FGenericWindow>& InWindow, const FIntVector2& InPosition)
        : Window(InWindow)
        , Width(0)
        , Height(0)
        , Position(InPosition)
    {
    }

    FWindowEvent(const TSharedRef<FGenericWindow>& InWindow, int32 InWidth, int32 InHeight)
        : Window(InWindow)
        , Width(InWidth)
        , Height(InHeight)
        , Position()
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