#pragma once
#include "CoreAPI.h"

#include "Core/Threading/InterlockedInt.h"

/* Base-class for intrusive ref-counted object */
class CORE_API CRefCounted
{
public:

    CRefCounted();
    virtual ~CRefCounted();

    int32 AddRef();
    int32 Release();

    int32 GetRefCount() const;

private:
    mutable InterlockedInt32 StrongReferences;
};
