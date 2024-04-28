#include "RHIResource.h"
#include "RHICommandList.h"

FRHIResource::FRHIResource()
    : StrongReferences(1)
    , State(static_cast<int32>(EState::Alive))
{
}

FRHIResource::~FRHIResource()
{
    CHECK(State.Load() == static_cast<int32>(EState::Deleted));
}

int32 FRHIResource::AddRef()
{
    CHECK(StrongReferences.Load() > 0);
    CHECK(State.Load() == static_cast<int32>(EState::Alive));
    ++StrongReferences;
    return StrongReferences.Load();
}

int32 FRHIResource::Release()
{
    CHECK(State.Load() == static_cast<int32>(EState::Alive));
    const int32 RefCount = --StrongReferences;
    CHECK(RefCount >= 0);

    if (RefCount < 1)
    {
        State = static_cast<int32>(EState::Deleted);
        GRHICommandExecutor.EnqueueResourceDeletion(this);
    }

    return RefCount;
}