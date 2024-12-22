#pragma once
#include "Core/Mac/ScopedAutoreleasePool.h"
#include <Foundation/Foundation.h>

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
