#include "MacPlatformMisc.h"

#include <Foundation/Foundation.h>


void FMacPlatformMisc::OutputDebugString(const FString& Message)
{
    NSLog(@"%s\n", Message.GetCString());
}
