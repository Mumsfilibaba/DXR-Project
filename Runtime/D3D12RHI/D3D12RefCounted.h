#pragma once
#include "Core/Threading/Atomic.h"
#include "D3D12RHI/D3D12Core.h"

class D3D12RHI_API FD3D12RefCounted
{
protected:
    FD3D12RefCounted()
        : StrongReferences(1)
    {
    }

    virtual ~FD3D12RefCounted()
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
        const int32 NewRefCount = --StrongReferences;
        CHECK(NewRefCount >= 0);

        if (NewRefCount < 1)
        {
            delete this;
        }

        return NewRefCount;
    }

    int32 GetRefCount() const
    {
        return StrongReferences.Load();
    }

protected:
    mutable FAtomicInt32 StrongReferences;
};