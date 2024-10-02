#include "RefCounted.h"

FRefCounted::FRefCounted()
    : NumRefs(1)
{
}

FRefCounted::~FRefCounted()
{
    CHECK(NumRefs.Load() == 0);
}

int32 FRefCounted::AddRef() const
{
    CHECK(NumRefs.Load() > 0);
    ++NumRefs;
    return NumRefs.Load();
}

int32 FRefCounted::Release() const
{
    const int32 RefCount = --NumRefs;
    CHECK(RefCount >= 0);

    if (RefCount < 1)
    {
        delete this;
    }

    return RefCount;
}

int32 FRefCounted::GetRefCount() const
{
    return NumRefs.Load();
}
