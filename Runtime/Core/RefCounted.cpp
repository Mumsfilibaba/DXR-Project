#include "RefCounted.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RefCounted

FRefCounted::FRefCounted()
    : StrongReferences(0)
{
    AddRef();
}

FRefCounted::~FRefCounted()
{
    Check(StrongReferences.Load() == 0);
}

int32 FRefCounted::AddRef()
{
    return ++StrongReferences;
}

int32 FRefCounted::Release()
{
    int32 NewRefCount = --StrongReferences;
    if (StrongReferences.Load() <= 0)
    {
        delete this;
    }

    return NewRefCount;
}

int32 FRefCounted::GetRefCount() const
{
    return StrongReferences.Load();
}
