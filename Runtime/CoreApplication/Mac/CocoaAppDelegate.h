#pragma once

#if PLATFORM_MACOS 
#include <Appkit/Appkit.h>

class CMacApplication;

@interface CCocoaAppDelegate : NSObject<NSApplicationDelegate>
{
    CMacApplication* Application;
}

- (id)init : (CMacApplication*)InApplication;

@end

#endif