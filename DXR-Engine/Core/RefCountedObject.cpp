#include "RefCountedObject.h"

RefCountedObject::RefCountedObject()
    : StrongReferences(0)
{
    AddRef();
}

int32 RefCountedObject::AddRef()
{
    return ++StrongReferences;
}

int32 RefCountedObject::Release()
{
    int32 NewRefCount = --StrongReferences;
    if (StrongReferences.Load() <= 0)
    {
        delete this;
    }

    return NewRefCount;
}

int32 RefCountedObject::GetRefCount() const
{
    return StrongReferences.Load();
}
