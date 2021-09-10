#pragma once
#include "Core.h"

#include "Core/Threading/InterlockedInt.h"

/* Base-class for intrusive refcounted object */
class RefCountedObject
{
public:
	
    RefCountedObject();
    virtual ~RefCountedObject();

    int32 AddRef();
    int32 Release();

    int32 GetRefCount() const;

private:
    mutable InterlockedInt32 StrongReferences;
};
