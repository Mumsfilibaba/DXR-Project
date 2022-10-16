#include "RefCounted.h"

FRefCounted::FRefCounted()
    : StrongReferences(1)
{ }

FRefCounted::~FRefCounted()
{
    CHECK(StrongReferences.Load() == 0);
}

int32 FRefCounted::AddRef()
{
    CHECK(StrongReferences.Load() > 0);
    ++StrongReferences;
    return StrongReferences.Load();
}

int32 FRefCounted::Release()
{
    const int32 RefCount = --StrongReferences;
    CHECK(RefCount >= 0);

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
