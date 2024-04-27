#include "RHIResource.h"

FRHIResource::FRHIResource()
    : StrongReferences(1)
{
}

int32 FRHIResource::AddRef()
{
    CHECK(StrongReferences.Load() > 0);
    ++StrongReferences;
    return StrongReferences.Load();
}

int32 FRHIResource::Release()
{
    const int32 RefCount = --StrongReferences;
    CHECK(RefCount >= 0);

    if (RefCount < 1)
    {
        delete this;
    }

    return RefCount;
}