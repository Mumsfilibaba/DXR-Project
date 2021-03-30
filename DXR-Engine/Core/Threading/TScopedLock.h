#pragma once
#include "Core.h"

template<typename TLock>
class TScopedLock
{
public:
    TScopedLock(TLock& InLock)
        : Lock(InLock)
    {
        Lock.Lock();
    }

    ~TScopedLock()
    {
        Lock.Unlock();
    }

private:
    TLock& Lock;
};