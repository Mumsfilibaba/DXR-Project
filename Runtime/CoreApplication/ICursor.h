#pragma once
#include "Core/Input/InputCodes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ECursor

enum class ECursor
{
    None       = 0,
    Arrow      = 1,
    TextInput  = 2,
    ResizeAll  = 3,
    ResizeEW   = 4,
    ResizeNS   = 5,
    ResizeNESW = 6,
    ResizeNWSE = 7,
    Hand       = 8,
    NotAllowed = 9,
};

class CPlatformWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ICursor

class ICursor
{
public:

    virtual ~ICursor() = default;

    /**
     * @brief: Set the cursor image
     *
     * @param Cursor: The new cursor icon
     */
    virtual void SetCursor(ECursor Cursor) = 0;

    /**
     * @brief: Sets the position of the cursor
     *
     * @param RelativeWindow: Window that the new position is relative to
     * @param x: The new x-coordinate
     * @param y: The new y-coordinate
     */
    virtual void SetPosition(CPlatformWindow* RelativeWindow, int32 x, int32 y) const = 0;

    /**
     * @brief: Sets the position of the cursor
     *
     * @param RelativeWindow: Window that the new position is relative to
     * @param OutX: Variable to store the x-coordinate in
     * @param OutY: Variable to store the y-coordinate in
     */
    virtual void GetPosition(CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY) const = 0;

    /**
     * @brief: Set the visibility of the cursor
     *
     * @param bIsVisible: True if the cursor should be visible, otherwise false
     */
    virtual void SetVisibility(bool bIsVisible) = 0;

    /** @return: Returns true if the mouse-cursor is visible, otherwise false */
    virtual bool IsVisible() const = 0;
};
