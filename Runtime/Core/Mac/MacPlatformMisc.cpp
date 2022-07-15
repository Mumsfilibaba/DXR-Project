#include "MacPlatformMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacPlatformMisc

void FMacPlatformMisc::OutputDebugString(const FString& Message)
{
    NSLog(@"%s\n", Message.CStr());
}
