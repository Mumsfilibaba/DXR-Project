#pragma once
#include "ScopedAutoreleasePool.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper macros

#ifndef NSSafeRelease
    #define NSSafeRelease(OutObject)   \
        do                             \
        {                              \
            if ((OutObject))           \
            {                          \
                [(OutObject) release]; \
                (OutObject) = nil;     \
            }                          \
        } while (false)
#endif

#ifndef NSRelease
    #define NSRelease(OutObject)   \
        do                         \
        {                          \
            [(OutObject) release]; \
            (OutObject) = nil;     \
        } while (false)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper to cast an NSObject

template<typename CastType>
inline CastType* NSClassCast(NSObject* Object)
{
    if ([Object isKindOfClass:[CastType class]])
    {
        return reinterpret_cast<CastType*>(Object);
    }
    else
    {
        return nullptr;
    }
}
