#pragma once
#include "Core/Misc/Asserts.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/Atomic.h"

struct FRefCountedBase : public FNonCopyable
{
protected:
    FRefCountedBase()
        : StrongReferences(1)
    {
    }

    virtual ~FRefCountedBase()
    {
        CHECK(StrongReferences.Load() == 0);
    }

public:
    int32 AddRef() const
    {
        CHECK(StrongReferences.Load() > 0);
        ++StrongReferences;
        return StrongReferences.Load();
    }

    int32 Release() const
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
    mutable AtomicInt32 StrongReferences;
};
