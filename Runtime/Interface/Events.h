#pragma once
#include "Core/Core.h"

#include "Core/Input/ModifierKeyState.h"
#include "Core/Input/InputCodes.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Interface/PlatformWindow.h"

struct SEvent
{
    FORCEINLINE SEvent()
        : bIsConsumed( false )
    {
    }

    /* If the key was down or nor */
    bool bIsConsumed;
};

struct SKeyEvent : public SEvent
{
    FORCEINLINE SKeyEvent( EKey InKeyCode, bool bInIsDown, bool bInIsRepeat, SModifierKeyState InModiferKeyState )
        : KeyCode( InKeyCode )
        , bIsDown( bInIsDown )
        , bIsRepeat( bInIsRepeat )
        , ModiferKeyState( InModiferKeyState )
    {
    }

    /* The KeyCode for this event */
    EKey KeyCode;

    /* If the key was down or not */
    bool bIsDown : 1;

    /* Is a repeated key event */
    bool bIsRepeat : 1;

    /* The other modifier keys that where down at the same time as the event */
    SModifierKeyState ModiferKeyState;
};


struct SKeyTypedEvent : public SEvent
{
    FORCEINLINE SKeyTypedEvent( uint32 InCharacter )
        : Character( InCharacter )
    {
    }

    FORCEINLINE char GetPrintableCharacter() const
    {
        return static_cast<char>(Character);
    }

    /* The character that where pressed, this is a ascii character in most cases */
    uint32 Character;
};

struct SMouseMovedEvent : public SEvent
{
    FORCEINLINE SMouseMovedEvent( int32 InX, int32 InY )
        : x( InX )
        , y( InY )
    {
    }

    int32 x;
    int32 y;
};

struct SMouseButtonEvent : public SEvent
{
    FORCEINLINE SMouseButtonEvent( EMouseButton InButton, bool bInIsDown, SModifierKeyState InModifiers )
        : Button( InButton )
        , bIsDown( bInIsDown )
        , Modifiers( InModifiers )
    {
    }

    /* The mouse button that for the event */
    EMouseButton Button;

    /* If the button where pressed or released */
    bool bIsDown;

    /* The modifier keys that also where pressed */
    SModifierKeyState Modifiers;
};

struct SMouseScrolledEvent : public SEvent
{
    FORCEINLINE SMouseScrolledEvent( float InHorizontalDelta, float InVerticalDelta )
        : HorizontalDelta( InHorizontalDelta )
        , VerticalDelta( InVerticalDelta )
    {
    }

    float HorizontalDelta;
    float VerticalDelta;
};

struct SWindowResizeEvent : public SEvent
{
    FORCEINLINE SWindowResizeEvent( const TSharedRef<CPlatformWindow>& InWindow, uint32 InWidth, uint32 InHeight )
        : Window( InWindow )
        , Width( InWidth )
        , Height( InHeight )
    {
    }

    TSharedRef<CPlatformWindow> Window;

    // Because of padding we can just use 32-bit integers
    uint32 Width;
    uint32 Height;
};

struct SWindowFocusChangedEvent : public SEvent
{
    FORCEINLINE SWindowFocusChangedEvent( const TSharedRef<CPlatformWindow>& InWindow, bool bInHasFocus )
        : Window( InWindow )
        , bHasFocus( bInHasFocus )
    {
    }

    TSharedRef<CPlatformWindow> Window;
    bool bHasFocus;
};

struct SWindowMovedEvent : public SEvent
{
    FORCEINLINE SWindowMovedEvent( const TSharedRef<CPlatformWindow>& InWindow, int32 InX, int32 InY )
        : Window( InWindow )
        , x( InX )
        , y( InY )
    {
    }

    TSharedRef<CPlatformWindow> Window;

    // Because of padding we can just use 32-bit integers
    int32 x;
    int32 y;
};

struct SWindowFrameMouseEvent : public SEvent
{
    FORCEINLINE SWindowFrameMouseEvent( const TSharedRef<CPlatformWindow>& InWindow, bool bInMouseEntered )
        : Window( InWindow )
        , bMouseEntered( bInMouseEntered )
    {
    }

    TSharedRef<CPlatformWindow> Window;
    bool bMouseEntered;
};

struct SWindowClosedEvent : public SEvent
{
    FORCEINLINE SWindowClosedEvent( const TSharedRef<CPlatformWindow>& InWindow )
        : Window( InWindow )
    {
    }

    TSharedRef<CPlatformWindow> Window;
};
