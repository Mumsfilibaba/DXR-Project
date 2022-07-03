#include "MacPlatformMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacPlatformMisc

void FMacPlatformMisc::OutputDebugString(const String& Message)
{
    NSLog(@"%s\n", Message.CStr());
}
