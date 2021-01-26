#pragma once
#include "Core.h"

class RefCountedObject
{
public:
    RefCountedObject();
    virtual ~RefCountedObject() = default;

    UInt32 AddRef();
    UInt32 Release();

    FORCEINLINE UInt32 GetRefCount() const
    {
        return StrongReferences;
    }

private:
    UInt32 StrongReferences;
};