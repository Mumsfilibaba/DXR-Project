#pragma once

#if PLATFORM_MACOS
#include "Core/Containers/SharedPtr.h"

#include "CoreApplication/Interface/PlatformCursor.h"

class CMacCursor final : public CPlatformCursor
{
public:

	static TSharedPtr<CMacCursor> Make();
	
	/* Public destructor for TSharedPtr */
	~CMacCursor() = default;
	
    /* Sets the type of cursor that is being used */
    virtual void SetCursor( ECursor Cursor ) override final;

    /* Sets the position of the cursor */
    virtual void SetPosition( CPlatformWindow* RelativeWindow, int32 x, int32 y ) const override final;

    /* Retrieve the cursor position of a window */
    virtual void GetPosition( CPlatformWindow* RelativeWindow, int32& OutX, int32& OutY ) const override final;

    /* Show or hide the mouse */
    virtual void SetVisibility( bool IsVisible ) override final;

private:

    CMacCursor()
        : CPlatformCursor()
    {
    }
};

#endif
