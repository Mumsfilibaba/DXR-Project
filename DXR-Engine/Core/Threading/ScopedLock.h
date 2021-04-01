#pragma once
#include "Core.h"

template<typename TLock>
class TScopedLock
{
public:
    TScopedLock(TScopedLock&&) = delete;
    TScopedLock(const TScopedLock&) = delete;

    TScopedLock& operator=(TScopedLock&&) = delete;
    TScopedLock& operator=(const TScopedLock&) = delete;

    TScopedLock(TLock& InLock)
        : Lock(InLock)
    {
        Lock.Lock();
    }

    ~TScopedLock()
    {
        Lock.Unlock();
    }

    TLock& GetLock() { return Lock; }

    const TLock& GetLock() const { return Lock; }

private:
    TLock& Lock;
};