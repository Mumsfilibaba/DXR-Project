#pragma once
#include "Core.h"

class RefCountedObject
{
public:
    RefCountedObject();
    virtual ~RefCountedObject() = default;

    uint32 AddRef();
    uint32 Release();

    uint32 GetRefCount() const { return StrongReferences; }

private:
    uint32 StrongReferences;
};