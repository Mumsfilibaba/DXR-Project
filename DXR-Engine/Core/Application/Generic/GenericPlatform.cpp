#include "GenericPlatform.h"

PlatformCallbacks* GenericPlatform::Callbacks = nullptr;

void GenericPlatform::SetCallbacks( PlatformCallbacks* InCallbacks )
{
    Callbacks = InCallbacks;
}

PlatformCallbacks* GenericPlatform::GetCallbacks()
{
    return nullptr;
}
