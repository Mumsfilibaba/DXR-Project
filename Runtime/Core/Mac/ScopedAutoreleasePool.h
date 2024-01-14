#pragma once
#include "Core/CoreDefines.h"

#include <Foundation/Foundation.h>

#define SCOPED_AUTORELEASE_POOL() const FScopedAutoreleasePool STRING_CONCAT(AutoReleasePool_, __LINE__)

class FScopedAutoreleasePool
{
public:
    FORCEINLINE FScopedAutoreleasePool()
        : Pool([NSAutoreleasePool new])
    {
    }

    FORCEINLINE ~FScopedAutoreleasePool()
    {
        [Pool release];
    }

private:
    NSAutoreleasePool* Pool;
};
