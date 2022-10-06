#pragma once
#include "D3D12Core.h"

#include "Core/Threading/AtomicInt.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RefCounted

class D3D12_RHI_API FD3D12RefCounted
{
protected:
    FD3D12RefCounted()
        : StrongReferences(1)
    { }

    virtual ~FD3D12RefCounted()
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