#pragma once

#if PLATFORM_MACOS 
#if defined(__OBJC__)

#include <Appkit/Appkit.h>

class CMacApplication;

@interface CCocoaAppDelegate : NSObject<NSApplicationDelegate>
{
    CMacApplication* Application;
}

- (id)init : (CMacApplication*)InApplication;

@end

#else

class CCocoaAppDelegate;

#endif
#endif
