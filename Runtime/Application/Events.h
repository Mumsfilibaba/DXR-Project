#pragma once
#include "Core/Core.h"
#include "Core/Input/ModifierKeyState.h"
#include "Core/Input/InputCodes.h"
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericWindow.h"

struct FEvent
{
    FORCEINLINE FEvent()
        : bIsConsumed(false)
    { }

    /** @brief - If the key was down or nor */
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

    /** @brief - The KeyCode for this event */
    EKey KeyCode;
    
    /** @brief - If the key was down or not */
    bool bIsDown : 1;
    
    /** @brief - Is a repeated key event */
    bool bIsRepeat : 1;
    
    /** @brief - The other modifier keys that where down at the same time as the event */
    FModifierKeyState ModiferKeyState;
};


struct FKeyCharEvent 
    : public FEvent
{
    FORCEINLINE FKeyCharEvent(uint32 InCharacter)
        : Character(InCharacter)
    { }

    FORCEINLINE CHAR GetPrintableCharacter() const
    {
        return static_cast<CHAR>(Character);
    }

    /** @brief - The character that where pressed, this is a ascii character in most cases */
    uint32 Character;
};


struct FMouseMovedEvent 
    : public FEvent
{
    FORCEINLINE FMouseMovedEvent(int32 InX, int32 InY)
        : X(InX)
        , Y(InY)
    { }

    /** @brief - Cursor X-Position */
    int32 X;

    /** @brief - Cursor Y-Position */
    int32 Y;
};


struct FHighPrecisionMouseEvent 
    : public FEvent
{
    FORCEINLINE FHighPrecisionMouseEvent(const FGenericWindowRef& InWindow, int32 InX, int32 InY)
        : Window(InWindow)
        , X(InX)
        , Y(InY)
    { }

    /** @brief - Window that the cursor moved inside */
    FGenericWindowRef Window;

    /** @brief - Cursor X-Position */
    int32 X;

    /** @brief - Cursor Y-Position */
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

    /** @brief - The mouse button that for the event */
    EMouseButton Button;
    
    /** @brief - If the button where pressed or released */
    bool bIsDown;
    
    /** @brief - The modifier keys that also where pressed */
    FModifierKeyState Modifiers;
};


struct FMouseScrolledEvent 
    : public FEvent
{
    FORCEINLINE FMouseScrolledEvent(float InHorizontalDelta, float InVerticalDelta)
        : HorizontalDelta(InHorizontalDelta)
        , VerticalDelta(InVerticalDelta)
    { }

    /** @brief - Horizontal mouse-wheel delta */
    float HorizontalDelta;
    
    /** @brief - Vertical mouse-wheel delta */
    float VerticalDelta;
};


struct FWindowResizeEvent 
    : public FEvent
{
    FORCEINLINE FWindowResizeEvent(const FGenericWindowRef& InWindow, uint32 InWidth, uint32 InHeight)
        : Window(InWindow)
        , Width(InWidth)
        , Height(InHeight)
    { }

    /** @brief - Window that got resized */
    FGenericWindowRef Window;

    /** @brief - New width of the window */
    uint32 Width;
    
    /** @brief - New height of the window */
    uint32 Height;
};


struct FWindowMovedEvent 
    : public FEvent
{
    FORCEINLINE FWindowMovedEvent(const FGenericWindowRef& InWindow, int32 InX, int32 InY)
        : Window(InWindow)
        , X(InX)
        , Y(InY)
    { }

    /** @brief - Window that moved */
    FGenericWindowRef Window;

    /** @brief - New x-position of the window */
    int32 X;
    
    /** @brief - New y-position of the window */
    int32 Y;
};


struct FWindowFrameMouseEvent 
    : public FEvent
{
    FORCEINLINE FWindowFrameMouseEvent(const FGenericWindowRef& InWindow, bool bInMouseEntered)
        : Window(InWindow)
        , bMouseEntered(bInMouseEntered)
    { }

    /** @brief - indow that is affected by the event */
    FGenericWindowRef Window;

    /** @brief - True if the cursor just entered the window, false otherwise */
    bool bMouseEntered;
};


struct FWindowClosedEvent 
    : public FEvent
{
    FORCEINLINE FWindowClosedEvent(const FGenericWindowRef& InWindow)
        : Window(InWindow)
    { }

    /** @brief - Window that got closed */
    FGenericWindowRef Window;
};
