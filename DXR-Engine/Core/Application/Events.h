#pragma once
#include "Core.h"

#include "Core/Application/InputCodes.h"
#include "Core/Application/Generic/GenericApplication.h"

struct KeyPressedEvent
{
    KeyPressedEvent(EKey InKey, bool InIsRepeat, const ModifierKeyState& InModifiers)
        : Key(InKey)
        , IsRepeat(InIsRepeat)
        , Modifiers(InModifiers)
    {
    }

    EKey Key;
    bool IsRepeat;
    ModifierKeyState Modifiers;
};

struct KeyReleasedEvent
{
    KeyReleasedEvent(EKey InKey, const ModifierKeyState& InModifiers)
        : Key(InKey)
        , Modifiers(InModifiers)
    {
    }

    EKey             Key;
    ModifierKeyState Modifiers;
};

struct KeyTypedEvent
{
    KeyTypedEvent(uint32 InCharacter)
        : Character(InCharacter)
    {
    }

    FORCEINLINE const char GetPrintableCharacter() const
    {
        return static_cast<char>(Character);
    }

    uint32 Character;
};

struct MouseMovedEvent
{
    MouseMovedEvent(int32 InX, int32 InY)
        : x(InX)
        , y(InY)
    {
    }

    int32 x;
    int32 y;
};

struct MousePressedEvent
{
    MousePressedEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
        : Button(InButton)
        , Modifiers(InModifiers)
    {
    }

    EMouseButton     Button;
    ModifierKeyState Modifiers;
};

struct MouseReleasedEvent
{
    MouseReleasedEvent(EMouseButton InButton, const ModifierKeyState& InModifiers)
        : Button(InButton)
        , Modifiers(InModifiers)
    {
    }

    EMouseButton     Button;
    ModifierKeyState Modifiers;
};


struct MouseScrolledEvent
{
    MouseScrolledEvent(float InHorizontalDelta, float InVerticalDelta)
        : HorizontalDelta(InHorizontalDelta)
        , VerticalDelta(InVerticalDelta)
    {
    }

    float HorizontalDelta;
    float VerticalDelta;
};

struct WindowResizeEvent
{
    WindowResizeEvent(const TRef<GenericWindow>& InWindow, uint16 InWidth, uint16 InHeight)
        : Window(InWindow)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    TRef<GenericWindow> Window;
    uint16 Width;
    uint16 Height;
};

struct WindowFocusChangedEvent
{
    WindowFocusChangedEvent(const TRef<GenericWindow>& InWindow, bool hasFocus)
        : Window(InWindow)
        , HasFocus(hasFocus)
    {
    }

    TRef<GenericWindow> Window;
    bool HasFocus;
};

struct WindowMovedEvent
{
    WindowMovedEvent(const TRef<GenericWindow>& InWindow, int16 x, int16 y)
        : Window(InWindow)
        , Position({ x, y })
    {
    }

    TRef<GenericWindow> Window;
    struct
    {
        int16 x;
        int16 y;
    } Position;
};

struct WindowMouseLeftEvent
{
    WindowMouseLeftEvent(const TRef<GenericWindow>& InWindow)
        : Window(InWindow)
    {
    }

    TRef<GenericWindow> Window;
};

struct WindowMouseEnteredEvent
{
    WindowMouseEnteredEvent(const TRef<GenericWindow>& InWindow)
        : Window(InWindow)
    {
    }

    TRef<GenericWindow> Window;
};

struct WindowClosedEvent
{
    WindowClosedEvent(const TRef<GenericWindow>& InWindow)
        : Window(InWindow)
    {
    }

    TRef<GenericWindow> Window;
};