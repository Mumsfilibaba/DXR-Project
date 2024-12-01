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

class FGenericWindow;

struct ICursor
{
    virtual ~ICursor() = default;

     /**
      * @brief Sets the type of cursor that is being used 
      * @param Cursor New cursor type to set
      */
    virtual void SetCursor(ECursor Cursor) = 0;

     /** 
      * @brief Sets the position of the cursor
      * @param x New x-position of the cursor
      * @param y New y-position of the cursor
      */
    virtual void SetPosition(int32 x, int32 y) = 0;

     /**
      * @brief Retrieve the cursor position of a window
      * @return Returns the cursor position
      */
    virtual FIntVector2 GetPosition() const = 0;

     /**
      * @brief Set the cursor visibility
      * @param bIsVisible The new visibility of the cursor, true to show it and false to hide it
      */
    virtual void SetVisibility(bool bIsVisible) = 0;

     /**
      * @return Returns the mouse visibility 
      */
    virtual bool IsVisible() const = 0;
};
