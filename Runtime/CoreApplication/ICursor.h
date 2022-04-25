#pragma once
#include "Core/Input/InputCodes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Cursor 

enum class ECursor
{
    None = 0,
    Arrow = 1,
    TextInput = 2,
    ResizeAll = 3,
    ResizeEW = 4,
    ResizeNS = 5,
    ResizeNESW = 6,
    ResizeNWSE = 7,
    Hand = 8,
    NotAllowed = 9,
};

class CPlatformWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for cursor

class ICursor
{
public:

    virtual ~ICursor() = default;

     /** @brief: Sets the type of cursor that is being used */
    virtual void SetCursor(ECursor Cursor) = 0;

     /** @brief: Sets the position of the cursor */
    virtual void SetPosition(CPlatformWindow* RelativeWindow, int32 x, int32 y) const = 0;

     /** @brief: Retrieve the cursor position of a window */
    virtual void GetPosition(CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY) const = 0;

     /** @brief: Show or hide the mouse */
    virtual void SetVisibility(bool bIsVisible) = 0;

     /** @brief: Retrieve the mouse visibility */
    virtual bool IsVisible() const = 0;
};
