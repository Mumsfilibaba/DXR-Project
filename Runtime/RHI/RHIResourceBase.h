#pragma once
#include "RHITypes.h"
#include "IRHIResource.h"

#include "Core/Threading/AtomicInt.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResource

class RHI_API CRHIResource : public IRHIResource
{
protected:

    CRHIResource()
        : StrongReferences(1)
    { }

    virtual ~CRHIResource() = default;

public:

    virtual int32 AddRef() override final
    {
        Assert(StrongReferences.Load() > 0);
        ++StrongReferences;
        return StrongReferences.Load();
    }

    virtual int32 Release() override final
    {
        const int32 RefCount = --StrongReferences;
        Assert(RefCount >= 0);

        if (RefCount < 1)
        {
            delete this;
        }

        return RefCount;
    }

protected:
    mutable AtomicInt32 StrongReferences;
};