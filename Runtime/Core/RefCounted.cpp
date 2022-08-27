#include "RefCounted.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRefCounted

FRefCounted::FRefCounted()
    : StrongReferences(1)
{ }

FRefCounted::~FRefCounted()
{
    Check(StrongReferences.Load() == 0);
}

int32 FRefCounted::AddRef()
{
    Check(StrongReferences.Load() > 0);
    ++StrongReferences;
    return StrongReferences.Load();
}

int32 FRefCounted::Release()
{
    const int32 RefCount = --StrongReferences;
    Check(RefCount >= 0);

    if (RefCount < 1)
    {
        delete this;
    }

    return RefCount;
}

int32 FRefCounted::GetRefCount() const
{
    return StrongReferences.Load();
}
