#include "RefCounted.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRefCounted

CRefCounted::CRefCounted()
    : StrongReferences(0)
{
    AddRef();
}

CRefCounted::~CRefCounted()
{
    Check(StrongReferences.Load() == 0);
}

int32 CRefCounted::AddRef()
{
    return ++StrongReferences;
}

int32 CRefCounted::Release()
{
    int32 NewRefCount = --StrongReferences;
    if (StrongReferences.Load() <= 0)
    {
        delete this;
    }

    return NewRefCount;
}

int32 CRefCounted::GetNumReferences() const
{
    return StrongReferences.Load();
}
