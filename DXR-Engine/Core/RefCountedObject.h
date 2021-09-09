#pragma once
#include "Core.h"

#include "Core/Threading/ThreadSafeInt.h"

class RefCountedObject
{
public:
    RefCountedObject();
    virtual ~RefCountedObject();

    int32 AddRef();
    int32 Release();

    int32 GetRefCount() const;

private:
    mutable ThreadSafeInt32 StrongReferences;
};