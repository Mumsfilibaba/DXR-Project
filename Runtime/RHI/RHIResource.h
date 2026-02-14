#pragma once
#include "Core/IRefCounted.h"
#include "Core/Threading/Atomic.h"
#include "RHI/RHICore.h"
#include "RHI/RHITypes.h"

class RHI_API FRHIResource : public IRefCounted
{
    enum class EState : int32
    {
        Unknown = 0,
        Alive,
        Deleted,
    };

public:
    FRHIResource(const FRHIResource&) = delete;
    FRHIResource& operator=(const FRHIResource&) = delete;

    FRHIResource();
    virtual ~FRHIResource();

    // IRefCounted Interface
    virtual int32 AddRef() const override;
    virtual int32 Release() const override;
    virtual int32 GetRefCount() const override;

private:
    mutable FAtomicInt32 StrongReferences;
    mutable FAtomicInt32 AliveState;
};
