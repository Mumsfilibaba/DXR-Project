#pragma once

#if PLATFORM_MACOS
#include "Core/CoreDefines.h"

#include <Foundation/Foundation.h>

#define SCOPED_AUTORELEASE_POOL() CScopedAutoreleasePool PREPROCESS_CONCAT(AutoReleasePool_, __LINE__)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CScopedAutoreleasePool - Creates a autorelease pool and releases it when destroyed

class CScopedAutoreleasePool
{
public:

    FORCEINLINE CScopedAutoreleasePool()
        : Pool([[NSAutoreleasePool alloc] init])
    {
    }

    FORCEINLINE ~CScopedAutoreleasePool()
    {
        [Pool release];
    }

private:
    NSAutoreleasePool* Pool;
};

#endif
