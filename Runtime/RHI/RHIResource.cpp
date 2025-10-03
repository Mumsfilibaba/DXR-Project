#include "RHI/RHIResource.h"
#include "RHI/RHICommandList.h"

FRHIResource::FRHIResource()
    : StrongReferences(1)
    , State(static_cast<int32>(EState::Alive))
{
}

FRHIResource::~FRHIResource()
{
    CHECK(StrongReferences.Load() == 0);
    CHECK(State.Load() == static_cast<int32>(EState::Deleted));
}

int32 FRHIResource::AddRef() const
{
    CHECK(StrongReferences.Load() > 0);
    CHECK(State.Load() == static_cast<int32>(EState::Alive));
    ++StrongReferences;
    return StrongReferences.Load();
}

int32 FRHIResource::Release() const
{
    CHECK(State.Load() == static_cast<int32>(EState::Alive));
    const int32 RefCount = --StrongReferences;
    CHECK(RefCount >= 0);

    if (RefCount < 1)
    {
        State = static_cast<int32>(EState::Deleted);

        // Delete immediately if we have not initialized the command-executor, this can happen if we fail to initialize the RHI
        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().EnqueueResourceDeletion(const_cast<FRHIResource*>(this));
        }
        else
        {
            delete this;
        }
    }

    return RefCount;
}

int32 FRHIResource::GetRefCount() const
{
    return StrongReferences.Load();
}
