#include "RefCountedObject.h"

RefCountedObject::RefCountedObject()
    : StrongReferences(0)
{
    AddRef();
}

UInt32 RefCountedObject::AddRef()
{
    return ++StrongReferences;
}

UInt32 RefCountedObject::Release()
{
    UInt32 NewRefCount = --StrongReferences;
    if (StrongReferences <= 0)
    {
        delete this;
    }

    return NewRefCount;
}
