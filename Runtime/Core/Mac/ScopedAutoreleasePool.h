#pragma once
#include "Core/CoreDefines.h"

#include <Foundation/Foundation.h>

#define SCOPED_AUTORELEASE_POOL() const CScopedAutoreleasePool PREPROCESS_CONCAT(AutoReleasePool_, __LINE__)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CScopedAutoreleasePool

class CScopedAutoreleasePool
{
public:

    FORCEINLINE CScopedAutoreleasePool()
        : Pool([NSAutoreleasePool new])
    { }

    FORCEINLINE ~CScopedAutoreleasePool()
    {
        [Pool release];
    }

private:
    NSAutoreleasePool* Pool;
};
