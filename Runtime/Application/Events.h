#pragma once
#include "Core/Input/ModifierKeyState.h"
#include "Core/Input/InputCodes.h"

struct FEvent
{
    FORCEINLINE FEvent()
        : bIsConsumed(false)
    { }

    bool bIsConsumed;
};

struct FKeyEvent 
    : public FEvent
{
    FORCEINLINE FKeyEvent(EKey InKeyCode, bool bInIsDown, bool bInIsRepeat, FModifierKeyState InModiferKeyState)
        : KeyCode(InKeyCode)
        , bIsDown(bInIsDown)
        , bIsRepeat(bInIsRepeat)
        , ModiferKeyState(InModiferKeyState)
    { }

    EKey KeyCode;

    bool bIsDown   : 1;
    bool bIsRepeat : 1;
    
    FModifierKeyState ModiferKeyState;
};

struct FKeyCharEvent 
    : public FEvent
{
    FORCEINLINE FKeyCharEvent(uint32 InCharacter)
        : Character(InCharacter)
    { }

    FORCEINLINE CHAR GetChar() const
    {
        return static_cast<CHAR>(Character);
    }

    uint32 Character;
};

struct FMouseMovedEvent 
    : public FEvent
{
    FORCEINLINE FMouseMovedEvent(int32 InX, int32 InY)
        : X(InX)
        , Y(InY)
    { }

    int32 X;
    int32 Y;
};

struct FHighPrecisionMouseEvent 
    : public FEvent
{
    FORCEINLINE FHighPrecisionMouseEvent(int32 InX, int32 InY)
        : X(InX)
        , Y(InY)
    { }

    int32 X;
    int32 Y;
};

struct FMouseButtonEvent 
    : public FEvent
{
    FORCEINLINE FMouseButtonEvent(EMouseButton InButton, bool bInIsDown, FModifierKeyState InModifiers)
        : Button(InButton)
        , bIsDown(bInIsDown)
        , Modifiers(InModifiers)
    { }

    EMouseButton Button;
    
    bool bIsDown;

    FModifierKeyState Modifiers;
};

struct FMouseScrolledEvent 
    : public FEvent
{
    FORCEINLINE FMouseScrolledEvent(float InHorizontalDelta, float InVerticalDelta)
        : HorizontalDelta(InHorizontalDelta)
        , VerticalDelta(InVerticalDelta)
    { }

    float HorizontalDelta;
    float VerticalDelta;
};

struct FWindowResizedEvent 
    : public FEvent
{
    FORCEINLINE FWindowResizedEvent(uint32 InWidth, uint32 InHeight)
        : Width(InWidth)
        , Height(InHeight)
    { }

    uint32 Width;
    uint32 Height;
};

struct FWindowMovedEvent 
    : public FEvent
{
    FORCEINLINE FWindowMovedEvent(int32 InX, int32 InY)
        : X(InX)
        , Y(InY)
    { }

    int32 X;
    int32 Y;
};
