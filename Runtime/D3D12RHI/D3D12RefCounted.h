#pragma once
#include "ID3D12RefCounted.h"

#include "Core/Threading/AtomicInt.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RefCounted

class D3D12_RHI_API CD3D12RefCounted : public ID3D12RefCounted
{
protected:

    CD3D12RefCounted()
        : StrongReferences(1)
    { }

    virtual ~CD3D12RefCounted() = default;

public:

    virtual int32 AddRef() override final
    {
        Check(StrongReferences.Load() > 0);
        ++StrongReferences;
        return StrongReferences.Load();
    }

    virtual int32 Release() override final
    {
        const int32 RefCount = --StrongReferences;
        Check(RefCount >= 0);

        if (RefCount < 1)
        {
            delete this;
        }

        return RefCount;
    }

protected:
    mutable AtomicInt32 StrongReferences;
};