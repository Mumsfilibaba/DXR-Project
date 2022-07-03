#pragma once
#include "Core/Core.h"

#include "Core/Input/ModifierKeyState.h"
#include "Core/Input/InputCodes.h"
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Generic/GenericWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SEvent 

struct SEvent
{
    FORCEINLINE SEvent()
        : bIsConsumed(false)
    { }

    /** @brief: If the key was down or nor */
    bool bIsConsumed;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SKeyEvent

struct SKeyEvent : public SEvent
{
    FORCEINLINE SKeyEvent(EKey InKeyCode, bool bInIsDown, bool bInIsRepeat, FModifierKeyState InModiferKeyState)
        : KeyCode(InKeyCode)
        , bIsDown(bInIsDown)
        , bIsRepeat(bInIsRepeat)
        , ModiferKeyState(InModiferKeyState)
    { }

    /** @brief: The KeyCode for this event */
    EKey KeyCode;
    
    /** @brief: If the key was down or not */
    bool bIsDown : 1;
    
    /** @brief: Is a repeated key event */
    bool bIsRepeat : 1;
    
    /** @brief: The other modifier keys that where down at the same time as the event */
    FModifierKeyState ModiferKeyState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SKeyCharEvent

struct SKeyCharEvent : public SEvent
{
    FORCEINLINE SKeyCharEvent(uint32 InCharacter)
        : Character(InCharacter)
    { }

    FORCEINLINE char GetPrintableCharacter() const
    {
        return static_cast<char>(Character);
    }

    /** @brief: The character that where pressed, this is a ascii character in most cases */
    uint32 Character;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMouseMovedEvent

struct SMouseMovedEvent : public SEvent
{
    FORCEINLINE SMouseMovedEvent(int32 InX, int32 InY)
        : X(InX)
        , Y(InY)
    { }

    /** @brief: Cursor X-Position */
    int32 X;

    /** @brief: Cursor Y-Position */
    int32 Y;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SHighPrecisionMouseEvent

struct SHighPrecisionMouseEvent : public SEvent
{
    FORCEINLINE SHighPrecisionMouseEvent(const TSharedRef<FGenericWindow>& InWindow, int32 InX, int32 InY)
        : Window(InWindow)
        , X(InX)
        , Y(InY)
    { }

    /** @brief: Window that the cursor moved inside */
    TSharedRef<FGenericWindow> Window;

    /** @brief: Cursor X-Position */
    int32 X;

    /** @brief: Cursor Y-Position */
    int32 Y;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMouseButtonEvent

struct SMouseButtonEvent : public SEvent
{
    FORCEINLINE SMouseButtonEvent(EMouseButton InButton, bool bInIsDown, FModifierKeyState InModifiers)
        : Button(InButton)
        , bIsDown(bInIsDown)
        , Modifiers(InModifiers)
    { }

    /** @brief: The mouse button that for the event */
    EMouseButton Button;
    
    /** @brief: If the button where pressed or released */
    bool bIsDown;
    
    /** @brief: The modifier keys that also where pressed */
    FModifierKeyState Modifiers;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMouseScrolledEvent

struct SMouseScrolledEvent : public SEvent
{
    FORCEINLINE SMouseScrolledEvent(float InHorizontalDelta, float InVerticalDelta)
        : HorizontalDelta(InHorizontalDelta)
        , VerticalDelta(InVerticalDelta)
    { }

    /** @brief: Horizontal mouse-wheel delta */
    float HorizontalDelta;
    
    /** @brief: Vertical mouse-wheel delta */
    float VerticalDelta;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowResizeEvent

struct SWindowResizeEvent : public SEvent
{
    FORCEINLINE SWindowResizeEvent(const TSharedRef<FGenericWindow>& InWindow, uint32 InWidth, uint32 InHeight)
        : Window(InWindow)
        , Width(InWidth)
        , Height(InHeight)
    { }

    /** @brief: Window that got resized */
    TSharedRef<FGenericWindow> Window;

    /** @brief: New width of the window */
    uint32 Width;
    
    /** @brief: New height of the window */
    uint32 Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowFocusChangedEvent

struct SWindowFocusChangedEvent : public SEvent
{
    FORCEINLINE SWindowFocusChangedEvent(const TSharedRef<FGenericWindow>& InWindow, bool bInHasFocus)
        : Window(InWindow)
        , bHasFocus(bInHasFocus)
    { }

    /** @brief: Window that had focus-status changed */
    TSharedRef<FGenericWindow> Window;

    /** @brief: Indicates weather the window got or lost focus */
    bool bHasFocus;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowMovedEvent

struct SWindowMovedEvent : public SEvent
{
    FORCEINLINE SWindowMovedEvent(const TSharedRef<FGenericWindow>& InWindow, int32 InX, int32 InY)
        : Window(InWindow)
        , X(InX)
        , Y(InY)
    { }

    /** @brief: Window that moved */
    TSharedRef<FGenericWindow> Window;

    /** @brief: New x-position of the window */
    int32 X;
    
    /** @brief: New y-position of the window */
    int32 Y;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowFrameMouseEvent

struct SWindowFrameMouseEvent : public SEvent
{
    FORCEINLINE SWindowFrameMouseEvent(const TSharedRef<FGenericWindow>& InWindow, bool bInMouseEntered)
        : Window(InWindow)
        , bMouseEntered(bInMouseEntered)
    { }

    /** @brief: Window that is affected by the event */
    TSharedRef<FGenericWindow> Window;

    /** @brief: True if the cursor just entered the window, false otherwise */
    bool bMouseEntered;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowClosedEvent

struct SWindowClosedEvent : public SEvent
{
    FORCEINLINE SWindowClosedEvent(const TSharedRef<FGenericWindow>& InWindow)
        : Window(InWindow)
    { }

    /** @brief: Window that got closed */
    TSharedRef<FGenericWindow> Window;
};
