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

class FGenericWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ICursor

struct ICursor
{
    virtual ~ICursor() = default;

     /**
      * @brief: Sets the type of cursor that is being used 
      *
      * @param Cursor: New cursor type to set
      */
    virtual void SetCursor(ECursor Cursor) = 0;

     /** 
      * @brief: Sets the position of the cursor 
      *
      * @param RelativeWindow: Window relative to the new position
      * @param x: New x-position of the cursor
      * @param y: New y-position of the cursor
      */
    virtual void SetPosition(FGenericWindow* RelativeWindow, int32 x, int32 y) const = 0;

     /**
      * @brief: Retrieve the cursor position of a window
      *
      * @param RelativeWindow: Window relative to the position
      * @param OutX: The x-position of the cursor
      * @param OutY: The y-position of the cursor
      */
    virtual void GetPosition(FGenericWindow* RelativeWindow, int32& OutX, int32& OutY) const = 0;

     /**
      * @brief: Set the cursor visibility
      * 
      * @param bIsVisible: The new visibility of the cursor, true to show it and false to hide it
      */
    virtual void SetVisibility(bool bIsVisible) = 0;

     /**
      * @return: Returns the mouse visibility 
      */
    virtual bool IsVisible() const = 0;
};
