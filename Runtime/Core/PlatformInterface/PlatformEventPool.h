#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Templates/Utility/NonCopyable.h"

struct IPlatformEvent;

class CORE_API FPlatformEventPool : public FNonCopyAndNonMovable
{
public:

    /** @brief Number of frames spanned by one low-water-mark window */
    static constexpr int32 PruneIntervalFrames = 60;

    /** @brief Upper bound on the events retained per reset mode between two prunes */
    static constexpr int32 MaxRetainedEvents = 16;

    /** @brief One bucket per reset mode, since Windows bakes the mode into the handle at creation */
    static constexpr int32 NumBuckets = 2;

    static FPlatformEventPool& Get();

public:
    IPlatformEvent* Acquire(bool bManualReset);
    void Release(IPlatformEvent* Event);

    void Tick();
    void Flush();

    int32 GetNumFreeEvents()                  const;
    int32 GetNumFreeEvents(bool bManualReset) const;

private:
    struct FBucket
    {
        TArray<IPlatformEvent*> FreeEvents;
        int32                   LowWaterMark;
    };

    static void DestroyEventList(TArray<IPlatformEvent*>& Events);

    FPlatformEventPool();
    ~FPlatformEventPool();

    void DetachEvents(FBucket& Bucket, int32 NumEvents, TArray<IPlatformEvent*>& OutDetached);

    FBucket                  Buckets[NumBuckets];
    int32                    FramesSincePrune;
    bool                     bPoolingEnabled;
    mutable FCriticalSection PoolCS;
};
