#pragma once
#include "Core/IRefCounted.h"
#include "Core/RefCountedBase.h"

struct CORE_API FRefCounted : public FRefCountedBase, public IRefCounted
{
    virtual ~FRefCounted() = default;

    virtual int32 AddRef()  const override { return FRefCountedBase::AddRef(); }
    virtual int32 Release() const override { return FRefCountedBase::Release(); }

    virtual int32 GetRefCount() const override { return FRefCountedBase::GetRefCount(); }
};
