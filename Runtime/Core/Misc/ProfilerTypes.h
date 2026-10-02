#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Time/Time.h"

/** @brief How many OS frames are stored on a scope when native stacks are captured. */
static constexpr int32 NUM_PROFILER_STACK_FRAMES = 16;

/** @brief How many finished frames the live editor keeps when it is not in a profile run. */
static constexpr int32 NUM_LIVE_PROFILER_FRAMES = 128;

struct FProfilerInterval
{
    FProfilerInterval()
        : Name(nullptr)
        , ThreadHandle(nullptr)
        , StartTimeStamp(0)
        , EndTimeStamp(0)
        , InclusiveNanoseconds(0)
        , ExclusiveNanoseconds(0)
        , ParentIndex(-1)
        , Depth(0)
        , StackDepth(0)
        , bInstant(false)
    {
        StackFrames.Fill(0);
    }

    const CHAR*                                     Name;
    void*                                           ThreadHandle;
    uint64                                          StartTimeStamp;
    uint64                                          EndTimeStamp;
    uint64                                          InclusiveNanoseconds;
    uint64                                          ExclusiveNanoseconds;
    int32                                           ParentIndex;
    int32                                           Depth;
    TStaticArray<uint64, NUM_PROFILER_STACK_FRAMES> StackFrames;
    int32                                           StackDepth;
    bool                                            bInstant;
};

struct FProfilerThreadFrame
{
    void*                     ThreadHandle = nullptr;
    TArray<FProfilerInterval> Intervals;
};

struct FProfilerFrame
{
    int32                        FrameIndex      = 0;
    uint64                       StartTimeStamp  = 0;
    uint64                       EndTimeStamp    = 0;
    float                        CpuMilliseconds = 0.0f;
    TArray<FProfilerThreadFrame> Threads;
};

template<typename FrameType>
struct TProfilerFrameRing
{
    void SetRetainAll(bool bInRetainAll)
    {
        bRetainAll = bInRetainAll;
    }

    NODISCARD bool RetainsAll() const
    {
        return bRetainAll;
    }

    void Clear()
    {
        Frames.Clear();
        WriteIndex = 0;
    }

    void Push(FrameType&& Frame)
    {
        if (bRetainAll)
        {
            Frames.Add(Move(Frame));
            return;
        }

        if (Frames.Size() < NUM_LIVE_PROFILER_FRAMES)
        {
            Frames.Add(Move(Frame));
            WriteIndex = Frames.Size() % NUM_LIVE_PROFILER_FRAMES;
            return;
        }

        Frames[WriteIndex] = Move(Frame);
        WriteIndex = (WriteIndex + 1) % NUM_LIVE_PROFILER_FRAMES;
    }

    NODISCARD int32 Num() const
    {
        return Frames.Size();
    }

    NODISCARD const FrameType* GetOldest(int32 OldestIndex) const
    {
        if (OldestIndex < 0 || OldestIndex >= Frames.Size())
        {
            return nullptr;
        }

        if (bRetainAll || Frames.Size() < NUM_LIVE_PROFILER_FRAMES)
        {
            return &Frames[OldestIndex];
        }

        return &Frames[(WriteIndex + OldestIndex) % NUM_LIVE_PROFILER_FRAMES];
    }

    NODISCARD const FrameType* GetLatest() const
    {
        return Num() > 0 ? GetOldest(Num() - 1) : nullptr;
    }

    template<typename Predicate>
    NODISCARD const FrameType* FindIf(Predicate Pred) const
    {
        for (int32 Index = 0; Index < Num(); ++Index)
        {
            const FrameType* Frame = GetOldest(Index);
            if (Frame && Pred(*Frame))
            {
                return Frame;
            }
        }

        return nullptr;
    }

private:
    TArray<FrameType> Frames;
    int32             WriteIndex = 0;
    bool              bRetainAll = false;
};

template<typename IntervalType>
void ApplyExclusiveTimesFromParentIndex(TArray<IntervalType>& Intervals)
{
    for (int32 Index = 0; Index < Intervals.Size(); ++Index)
    {
        Intervals[Index].ExclusiveNanoseconds = Intervals[Index].InclusiveNanoseconds;
    }

    for (int32 Index = 0; Index < Intervals.Size(); ++Index)
    {
        const int32 ParentIndex = Intervals[Index].ParentIndex;
        if (ParentIndex < 0 || ParentIndex >= Intervals.Size())
        {
            continue;
        }

        const uint64 ChildInclusive = Intervals[Index].InclusiveNanoseconds;
        uint64& Exclusive = Intervals[ParentIndex].ExclusiveNanoseconds;
        Exclusive = ChildInclusive >= Exclusive ? 0 : (Exclusive - ChildInclusive);
    }
}

CORE_API void FinalizeProfilerIntervals(TArray<FProfilerInterval>& Intervals, uint64 Frequency);
CORE_API void SortProfilerThreadFrames(TArray<FProfilerThreadFrame>& Threads);
CORE_API void CollectProfilerCallers(const TArray<FProfilerInterval>& Intervals, int32 Index, TArray<int32>& OutCallers);

FORCEINLINE uint64 ProfilerTicksToNanoseconds(uint64 DeltaTicks, uint64 Frequency)
{
    if (Frequency == 0)
    {
        return 0;
    }

    return Time::FromSeconds(DeltaTicks) / Frequency;
}
