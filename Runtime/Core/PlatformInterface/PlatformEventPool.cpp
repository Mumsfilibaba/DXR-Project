#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformEvent.h"
#include "Core/PlatformInterface/PlatformEventPool.h"
#include "Core/Threading/ScopedLock.h"

static TAutoConsoleVariable<bool> CVarEnablePlatformEventPooling(
    "Threading.EnablePlatformEventPooling",
    "Recycle platform events through the event pool instead of destroying them on release",
    true);

FPlatformEventPool::FPlatformEventPool()
    : Buckets()
    , FramesSincePrune(0)
    , bPoolingEnabled(true)
{
    for (FBucket& Bucket : Buckets)
    {
        Bucket.LowWaterMark = 0;
    }
}

FPlatformEventPool::~FPlatformEventPool() = default;

FPlatformEventPool& FPlatformEventPool::Get()
{
    static FPlatformEventPool* PlatformEventPool = new FPlatformEventPool();
    return *PlatformEventPool;
}

IPlatformEvent* FPlatformEventPool::Acquire(bool bManualReset)
{
    {
        TScopedLock Lock(PoolCS);

        FBucket& Bucket = Buckets[bManualReset ? 1 : 0];
        if (bPoolingEnabled)
        {
            if (!Bucket.FreeEvents.IsEmpty())
            {
                IPlatformEvent* Recycled = Bucket.FreeEvents.Last();
                Bucket.FreeEvents.Pop();

                Bucket.LowWaterMark = Math::Min(Bucket.LowWaterMark, Bucket.FreeEvents.Size());

                // The previous owner may have triggered it and never waited, so it comes back unsignalled
                Recycled->Reset();
                return Recycled;
            }

            // Demand outran the pool, so nothing retained during this window is surplus
            Bucket.LowWaterMark = 0;
        }
    }

    return FPlatformEvent::CreateUnpooled(bManualReset);
}

void FPlatformEventPool::Release(IPlatformEvent* Event)
{
    if (!Event)
    {
        return;
    }

    {
        TScopedLock Lock(PoolCS);

        // Releasing an event a thread is still inside Wait on stays a caller bug, the same one that
        // was a use-after-free before pooling. It now shows up as a spurious wake instead of a crash.
        FBucket& Bucket = Buckets[Event->IsManualReset() ? 1 : 0];
        if (bPoolingEnabled && Bucket.FreeEvents.Size() < MaxRetainedEvents)
        {
            Bucket.FreeEvents.Add(Event);
            return;
        }
    }

    FPlatformEvent::DestroyUnpooled(Event);
}

void FPlatformEventPool::Tick()
{
    TArray<IPlatformEvent*> Pruned;
    {
        TScopedLock Lock(PoolCS);

        bPoolingEnabled = CVarEnablePlatformEventPooling.GetValue();
        FramesSincePrune++;

        if (bPoolingEnabled && FramesSincePrune < PruneIntervalFrames)
        {
            return;
        }

        FramesSincePrune = 0;

        for (FBucket& Bucket : Buckets)
        {
            DetachEvents(Bucket, bPoolingEnabled ? Bucket.LowWaterMark : Bucket.FreeEvents.Size(), Pruned);
        }
    }

    DestroyEventList(Pruned);
}

void FPlatformEventPool::Flush()
{
    TArray<IPlatformEvent*> Pruned;

    {
        TScopedLock Lock(PoolCS);

        for (FBucket& Bucket : Buckets)
        {
            DetachEvents(Bucket, Bucket.FreeEvents.Size(), Pruned);
        }
    }

    DestroyEventList(Pruned);
}

int32 FPlatformEventPool::GetNumFreeEvents() const
{
    TScopedLock Lock(PoolCS);
    return Buckets[0].FreeEvents.Size() + Buckets[1].FreeEvents.Size();
}

int32 FPlatformEventPool::GetNumFreeEvents(bool bManualReset) const
{
    TScopedLock Lock(PoolCS);
    return Buckets[bManualReset ? 1 : 0].FreeEvents.Size();
}

void FPlatformEventPool::DetachEvents(FBucket& Bucket, int32 NumEvents, TArray<IPlatformEvent*>& OutDetached)
{
    for (int32 Index = 0; Index < NumEvents && !Bucket.FreeEvents.IsEmpty(); Index++)
    {
        OutDetached.Add(Bucket.FreeEvents.Last());
        Bucket.FreeEvents.Pop();
    }

    Bucket.LowWaterMark = Bucket.FreeEvents.Size();
}

void FPlatformEventPool::DestroyEventList(TArray<IPlatformEvent*>& Events)
{
    for (IPlatformEvent* Event : Events)
    {
        FPlatformEvent::DestroyUnpooled(Event);
    }

    Events.Clear();
}
