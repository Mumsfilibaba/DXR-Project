#pragma once

#if PLATFORM_MAC
#include <Foundation/Foundation.h>

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

#endif