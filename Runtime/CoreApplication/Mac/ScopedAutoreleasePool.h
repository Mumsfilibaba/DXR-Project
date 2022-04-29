#pragma once
#include "Core/CoreDefines.h"

#include <Foundation/Foundation.h>

#define SCOPED_AUTORELEASE_POOL() CScopedAutoreleasePool PREPROCESS_CONCAT(AutoReleasePool_, __LINE__)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CScopedAutoreleasePool

class CScopedAutoreleasePool
{
public:

    FORCEINLINE CScopedAutoreleasePool()
        : Pool([[NSAutoreleasePool alloc] init])
    { }

    FORCEINLINE ~CScopedAutoreleasePool()
    {
        [Pool release];
    }

private:
    NSAutoreleasePool* Pool;
};
