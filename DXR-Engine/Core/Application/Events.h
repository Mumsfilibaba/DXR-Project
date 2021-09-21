#pragma once
#include "Core.h"

#include "Core/Input/InputCodes.h"
#include "Core/Application/ModifierKeyState.h"
#include "Core/Application/Generic/GenericWindow.h"

struct KeyPressedEvent
{
    KeyPressedEvent( EKey InKey, bool InIsRepeat, SModifierKeyState InModifiers )
        : Key( InKey )
        , IsRepeat( InIsRepeat )
        , Modifiers( InModifiers )
    {
    }

    EKey Key;
    bool IsRepeat;
    SModifierKeyState Modifiers;
};

struct KeyReleasedEvent
{
    KeyReleasedEvent( EKey InKey, SModifierKeyState InModifiers )
        : Key( InKey )
        , Modifiers( InModifiers )
    {
    }

    EKey              Key;
    SModifierKeyState Modifiers;
};

struct KeyTypedEvent
{
    KeyTypedEvent( uint32 InCharacter )
        : Character( InCharacter )
    {
    }

    FORCEINLINE char GetPrintableCharacter() const
    {
        return static_cast<char>(Character);
    }

    uint32 Character;
};

struct MouseMovedEvent
{
    MouseMovedEvent( int32 InX, int32 InY )
        : x( InX )
        , y( InY )
    {
    }

    int32 x;
    int32 y;
};

struct MousePressedEvent
{
    MousePressedEvent( EMouseButton InButton, SModifierKeyState InModifiers )
        : Button( InButton )
        , Modifiers( InModifiers )
    {
    }

    EMouseButton      Button;
    SModifierKeyState Modifiers;
};

struct MouseReleasedEvent
{
    MouseReleasedEvent( EMouseButton InButton, SModifierKeyState InModifiers )
        : Button( InButton )
        , Modifiers( InModifiers )
    {
    }

    EMouseButton      Button;
    SModifierKeyState Modifiers;
};


struct MouseScrolledEvent
{
    MouseScrolledEvent( float InHorizontalDelta, float InVerticalDelta )
        : HorizontalDelta( InHorizontalDelta )
        , VerticalDelta( InVerticalDelta )
    {
    }

    float HorizontalDelta;
    float VerticalDelta;
};

struct WindowResizeEvent
{
    WindowResizeEvent( const TSharedRef<CGenericWindow>& InWindow, uint16 InWidth, uint16 InHeight )
        : Window( InWindow )
        , Width( InWidth )
        , Height( InHeight )
    {
    }

    TSharedRef<CGenericWindow> Window;
    uint16 Width;
    uint16 Height;
};

struct WindowFocusChangedEvent
{
    WindowFocusChangedEvent( const TSharedRef<CGenericWindow>& InWindow, bool hasFocus )
        : Window( InWindow )
        , HasFocus( hasFocus )
    {
    }

    TSharedRef<CGenericWindow> Window;
    bool HasFocus;
};

struct WindowMovedEvent
{
    WindowMovedEvent( const TSharedRef<CGenericWindow>& InWindow, int16 x, int16 y )
        : Window( InWindow )
        , Position( { x, y } )
    {
    }

    TSharedRef<CGenericWindow> Window;
    struct
    {
        int16 x;
        int16 y;
    } Position;
};

struct WindowMouseLeftEvent
{
    WindowMouseLeftEvent( const TSharedRef<CGenericWindow>& InWindow )
        : Window( InWindow )
    {
    }

    TSharedRef<CGenericWindow> Window;
};

struct WindowMouseEnteredEvent
{
    WindowMouseEnteredEvent( const TSharedRef<CGenericWindow>& InWindow )
        : Window( InWindow )
    {
    }

    TSharedRef<CGenericWindow> Window;
};

struct WindowClosedEvent
{
    WindowClosedEvent( const TSharedRef<CGenericWindow>& InWindow )
        : Window( InWindow )
    {
    }

    TSharedRef<CGenericWindow> Window;
};
