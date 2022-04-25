#pragma once

#if PLATFORM_MACOS
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/Interface/PlatformCursor.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation for cursor interface

class CMacCursor final : public CPlatformCursor
{
public:

	static TSharedPtr<CMacCursor> Make();
	
	 /** @brief: Public destructor for TSharedPtr */
	~CMacCursor() = default;
	
     /** @brief: Sets the type of cursor that is being used */
    virtual void SetCursor(ECursor Cursor) override final;

     /** @brief: Sets the position of the cursor */
    virtual void SetPosition(CPlatformWindow* RelativeWindow, int32 x, int32 y) const override final;

     /** @brief: Retrieve the cursor position of a window */
    virtual void GetPosition(CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY) const override final;

     /** @brief: Show or hide the mouse */
    virtual void SetVisibility(bool bVisible) override final;

private:

    CMacCursor()
        : CPlatformCursor()
    { }
};

#endif
