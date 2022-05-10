#pragma once
#include <Appkit/Appkit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaAppDelegate

@interface CCocoaAppDelegate : NSObject<NSApplicationDelegate>
{
    CMacApplication* Application;
}

- (instancetype)init : (CMacApplication*)InApplication NS_DESIGNATED_INITIALIZER;

@end
