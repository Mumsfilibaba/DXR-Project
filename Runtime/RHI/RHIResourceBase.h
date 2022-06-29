#pragma once
#include "RHITypes.h"
#include "IRHIResource.h"

#include "Core/Threading/AtomicInt.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIResource

class RHI_API FRHIResource : public IRHIResource
{
protected:

    FRHIResource()
        : StrongReferences(1)
    { }

    virtual ~FRHIResource() = default;

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