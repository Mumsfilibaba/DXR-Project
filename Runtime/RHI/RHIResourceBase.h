#pragma once
#include "RHITypes.h"
#include "RHICore.h"

#include "Core/IRefCounted.h"
#include "Core/Threading/AtomicInt.h"

class RHI_API FRHIResource 
    : public IRefCounted
{
protected:
    FRHIResource()
        : StrongReferences(1)
    { }

    virtual ~FRHIResource() = default;

public:
    virtual int32 AddRef() override
    {
        CHECK(StrongReferences.Load() > 0);
        ++StrongReferences;
        return StrongReferences.Load();
    }

    virtual int32 Release() override
    {
        const int32 RefCount = --StrongReferences;
        CHECK(RefCount >= 0);

        if (RefCount < 1)
        {
            delete this;
        }

        return RefCount;
    }

    virtual int32 GetRefCount() const override
    {
        return StrongReferences.Load();
    }

protected:
    mutable FAtomicInt32 StrongReferences;
};