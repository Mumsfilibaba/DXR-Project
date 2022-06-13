#include "MacPlatformMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacPlatformMisc

void CMacPlatformMisc::OutputDebugString(const String& Message)
{
    NSLog(@"%s\n", Message.CStr());
}
