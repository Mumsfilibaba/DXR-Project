#pragma once

#if defined(PLATFORM_MACOS) && defined(__OBJC__)
#include "CoreDefines.h"

#include <Foundation/Foundation.h>

#define SCOPED_AUTORELEASE_POOL() CScopedAutoreleasePool PREPROCESS_CONCAT(AutoReleasePool_, __LINE__)

class CScopedAutoreleasePool
{
public:
    FORCEINLINE CScopedAutoreleasePool()
        : Pool(nullptr)
    {
        Pool = [[NSAutoreleasePool alloc] init];
    }

    FORCEINLINE ~CScopedAutoreleasePool()
    {
        [Pool release];
    }

private:
    NSAutoreleasePool* Pool;
};

#endif
