#include "MacPlatformMisc.h"

#include <Foundation/Foundation.h>

void FMacPlatformMisc::OutputDebugString(const CHAR* Message)
{
    NSLog(@"%s", Message);
}
