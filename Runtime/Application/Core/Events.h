#pragma once
#include "Core/Input/ModifierKeyState.h"
#include "Core/Input/InputCodes.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Core/Core.h"

class FWindow;

class FWindowResizedEvent
{
public:
    FWindowResizedEvent(uint32 InWidth, uint32 InHeight)
        : Width(InWidth)
        , Height(InHeight)
    { }

    uint32 GetWidth() const
    {
        return Width;
    }

    uint32 GetHeight() const
    {
        return Height;
    }

private:
    uint32 Width;
    uint32 Height;
};

class FWindowMovedEvent
{
public:
    FWindowMovedEvent(FIntVector2 InPosition)
        : Position(InPosition)
    { }

    const FIntVector2& GetPosition() const
    {
        return Position;
    }

private:
    FIntVector2 Position;
};


class APPLICATION_API FResponse
{
public:
    static FResponse Handled()
    {
        return FResponse(true);
    }

    static FResponse Unhandled()
    {
        return FResponse(false);
    }

public:
    FResponse(bool bInIsHandled)
        : bIsHandled(bInIsHandled)
        , bCaptureMouse(false)
        , bReleaseCapture(false)
    {
    }

    bool IsEventHandled() const
    {
        return bIsHandled;
    }

    bool ShouldCaptureMouse() const
    {
        return bReleaseCapture;
    }

    bool ShouldReleaseMouse() const
    {
        return bReleaseCapture;
    }

    FResponse& CaptureMouse()
    {
        bCaptureMouse   = true;
        bReleaseCapture = false;
        return *this;
    }

    FResponse& ReleaseMouse()
    {
        bCaptureMouse   = false;
        bReleaseCapture = true;
        return *this;
    }

private:
    uint32 bIsHandled      : 1;
    uint32 bCaptureMouse   : 1;
    uint32 bReleaseCapture : 1;
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
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys)
        : FInputEvent(ModifierKeys)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , Button(MouseButton_Unknown)
        , bIsVerticalScrollDelta(false)
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys, EMouseButton InButton)
        : FInputEvent(ModifierKeys)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(0.0f)
        , Button(InButton)
        , bIsVerticalScrollDelta(false)
    {
    }

    FMouseEvent(const FIntVector2& InCursorPosition, const FModifierKeyState& ModifierKeys, float InScrollDelta, bool bInIsVerticalScrollDelta)
        : FInputEvent(ModifierKeys)
        , CursorPosition(InCursorPosition)
        , ScrollDelta(InScrollDelta)
        , Button(MouseButton_Unknown)
        , bIsVerticalScrollDelta(bInIsVerticalScrollDelta)
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

    FControllerEvent(EControllerButton InButton, uint32 InControllerIndex)
        : FInputEvent()
        , Button(InButton)
        , ControllerIndex(ControllerIndex)
        , AnalogSource(EControllerAnalog::Unknown)
        , AnalogValue(0.0f)
    {
    }

    FControllerEvent(EControllerAnalog InAnalogSource, float InAnalogValue, uint32 InControllerIndex)
        : FInputEvent()
        , Button(EControllerButton::Unknown)
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

private:
    EControllerButton Button;
    EControllerAnalog AnalogSource;
    float             AnalogValue;
    uint32            ControllerIndex;
};