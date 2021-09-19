#pragma once

#if defined(PLATFORM_MACOS)
#include "CoreDefines.h"

#include <CoreGraphics/CoreGraphics.h>

#if defined(__OBJC__)
@class NSNotification;
@class NSWindow;
#else
class NSNotification;
class NSWindow;
#endif

struct SNotification
{
    /* The NSNotification */
    NSNotification* Notification = nullptr;

    /* A valid NSWindow pointer if the notification is a window notification */
    NSWindow* Window = nullptr;

    /* The size of the NSWindow */
    CGSize Size;

    /* The position of the NSWindow */
	CGPoint Position;
	
	FORCEINLINE bool IsValid() const
	{
		return (Notification != nullptr);
	}
};

#endif
