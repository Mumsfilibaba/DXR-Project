#pragma once
#include "Core.h"

#include "Core/Threading/InterlockedInt.h"

/* Base-class for intrusive ref-counted object */
class CRefCounted
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
