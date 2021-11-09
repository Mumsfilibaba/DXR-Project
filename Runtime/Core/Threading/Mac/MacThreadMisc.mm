#if PLATFORM_MACOS
#include "MacThreadMisc.h"

#include <Foundation/Foundation.h>

uint32 CMacThreadMisc::GetNumProcessors()
{
    NSUInteger NumProcessors = [[NSProcessInfo processInfo] processorCount];
    return static_cast<uint32>(NumProcessors);
}

#endif