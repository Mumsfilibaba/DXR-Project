#pragma once
#include "MetalCore.h"
#include "Core/Threading/Atomic.h"

class FMetalRefCounted
{
protected:
    FMetalRefCounted()
        : StrongReferences(1)
    {
    }

    virtual ~FMetalRefCounted()
    {
        CHECK(StrongReferences.Load() == 0);
    }

public:
    int32 AddRef()
    {
        CHECK(StrongReferences.Load() > 0);
        ++StrongReferences;
        return StrongReferences.Load();
    }

    int32 Release()
    {
        const int32 RefCount = --StrongReferences;
        CHECK(RefCount >= 0);

        if (RefCount < 1)
        {
            delete this;
        }

        return RefCount;
    }

    int32 GetRefCount() const
    {
        return StrongReferences.Load();
    }

protected:
    mutable FAtomicInt32 StrongReferences;
};