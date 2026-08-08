#pragma once
#include "Core/Containers/Array.h"
#include "Core/Misc/Asserts.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/ScopedLock.h"

template<typename ObjectType>
class TD3D12RecyclePool : FNonCopyable
{
public:
    TD3D12RecyclePool()  = default;

    ~TD3D12RecyclePool()
    {
        CHECK(AllObjects.IsEmpty() && "DestroyAll must be called before the owning object is destroyed");
    }

    template<typename CreateFunctorType>
    ObjectType* Acquire(CreateFunctorType&& Create)
    {
        TScopedLock Lock(PoolCS);

        if (!FreeObjects.IsEmpty())
        {
            ObjectType* Recycled = FreeObjects.Last();
            FreeObjects.Pop();
            return Recycled;
        }

        ObjectType* Created = Create(AllObjects.Size());
        if (Created)
        {
            AllObjects.Add(Created);
        }

        return Created;
    }

    void Release(ObjectType* Object)
    {
        CHECK(Object != nullptr);

        TScopedLock Lock(PoolCS);
        FreeObjects.Add(Object);
    }

    template<typename PredicateType>
    void PruneFree(int32 MinRetained, PredicateType&& ShouldPrune)
    {
        TScopedLock Lock(PoolCS);

        for (int32 Index = FreeObjects.Size() - 1; Index >= 0; Index--)
        {
            if (AllObjects.Size() <= MinRetained)
            {
                break;
            }

            ObjectType* Candidate = FreeObjects[Index];
            if (!ShouldPrune(Candidate))
            {
                continue;
            }

            FreeObjects.RemoveAtSwap(Index);
            AllObjects.RemoveSingleSwap(Candidate);
            delete Candidate;
        }
    }

    template<typename FunctorType>
    void ForEachTracked(FunctorType&& Functor)
    {
        TScopedLock Lock(PoolCS);

        for (ObjectType* Object : AllObjects)
        {
            Functor(*Object);
        }
    }

    void DestroyAll()
    {
        TScopedLock Lock(PoolCS);

        CHECK(FreeObjects.Size() == AllObjects.Size() && "Every pooled object must be released before the pool is destroyed");

        for (ObjectType* Object : AllObjects)
        {
            delete Object;
        }

        FreeObjects.Clear();
        AllObjects.Clear();
    }

    int32 GetNumTracked() const
    {
        TScopedLock Lock(PoolCS);
        return AllObjects.Size();
    }

    int32 GetNumFree() const
    {
        TScopedLock Lock(PoolCS);
        return FreeObjects.Size();
    }

private:
    TArray<ObjectType*>      AllObjects;
    TArray<ObjectType*>      FreeObjects;
    mutable FCriticalSection PoolCS;
};
