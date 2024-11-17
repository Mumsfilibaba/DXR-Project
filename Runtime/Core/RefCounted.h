#pragma once
#include "IRefCounted.h"
#include "Core/Threading/Atomic.h"

class CORE_API FRefCounted : public IRefCounted
{
public:
    FRefCounted(const FRefCounted&) = delete;
    FRefCounted& operator=(const FRefCounted&) = delete;

    FRefCounted();
    virtual ~FRefCounted();

    virtual int32 AddRef() const override;

    virtual int32 Release() const override;

    virtual int32 GetRefCount() const override;

private:
    mutable FAtomicInt32 NumRefs;
};

