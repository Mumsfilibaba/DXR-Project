#pragma once
#include "RHICore.h"
#include "RHITypes.h"
#include "Core/IRefCounted.h"
#include "Core/Threading/AtomicInt.h"

class RHI_API FRHIResource : public IRefCounted
{
    enum class EState : int32
    {
        Unknown = 0,
        Alive,
        Deleted,
    };

public:
    FRHIResource();
    virtual ~FRHIResource();

    virtual int32 AddRef() override;
    virtual int32 Release() override;

    virtual int32 GetRefCount() const override
    {
        return StrongReferences.Load();
    }

private:
    FAtomicInt32 StrongReferences;
    FAtomicInt32 State;
};