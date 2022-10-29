#pragma once
#include "IRefCounted.h"

#include "Core/Threading/AtomicInt.h"
#include "Core/Templates/TypeTraits.h"

class CORE_API FRefCounted 
    : public IRefCounted
{
protected:
    FRefCounted();
    virtual ~FRefCounted();

public:
    virtual int32 AddRef()            override;
    virtual int32 Release()           override;
    virtual int32 GetRefCount() const override;

private:
    FAtomicInt32 NumRefs;
};

