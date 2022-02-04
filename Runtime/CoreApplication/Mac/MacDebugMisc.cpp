#if PLATFORM_MACOS
#include "MacDebugMisc.h"

#include <Foundation/Foundation.h>

void CMacDebugMisc::OutputDebugString(const String& Message)
{ 
    NSLog(@"%s\n", Message.CStr());
}

#endif
