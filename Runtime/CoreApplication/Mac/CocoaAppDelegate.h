#pragma once
#include <Appkit/Appkit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaAppDelegate

@interface CCocoaAppDelegate : NSObject<NSApplicationDelegate>
{
    CMacApplication* Application;
}

- (id)init : (CMacApplication*)InApplication;

@end