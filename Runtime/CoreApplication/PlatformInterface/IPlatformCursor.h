#pragma once
#include "Core/Math/IntVector2.h"

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

struct IPlatformWindow;

struct IPlatformCursor
{
    virtual ~IPlatformCursor() = default;

    /**
     * @brief Sets the type of cursor that is being used.
     * 
     * @param Cursor New cursor type to set.
     */
    virtual void SetCursor(ECursor Cursor) = 0;

    /** 
     * @brief Sets the position of the cursor.
     * 
     * @param x New x-position of the cursor.
     * @param y New y-position of the cursor.
     */
    virtual void SetPosition(int32 x, int32 y) = 0;

    /**
     * @brief Retrieves the current position of the cursor.
     * 
     * @return The cursor position.
     */
    virtual IntVector2 GetPosition() const = 0;

    /**
     * @brief Sets the cursor visibility.
     * 
     * @param bVisible True to show the cursor and false to hide it.
     */
    virtual void SetVisibility(bool bVisible) = 0;

    /**
     * @brief Retrieves the cursor visibility.
     * 
     * @return True if the cursor is visible, otherwise false.
     */
    virtual bool IsVisible() const = 0;
};
