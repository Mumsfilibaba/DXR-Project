#include "Core/Misc/ProfilerTypes.h"
#include "Core/Containers/Map.h"
#include "Core/Math/Math.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ThreadManager.h"
#include "Core/Time/Time.h"

static int32 CompareThreadNames(const String& Left, const String& Right)
{
    int32 LeftIndex  = 0;
    int32 RightIndex = 0;

    while (LeftIndex < Left.Length() && RightIndex < Right.Length())
    {
        if (CharTraits::IsDigit(Left[LeftIndex]) && CharTraits::IsDigit(Right[RightIndex]))
        {
            while ((LeftIndex + 1) < Left.Length() && Left[LeftIndex] == '0' && CharTraits::IsDigit(Left[LeftIndex + 1]))
            {
                ++LeftIndex;
            }

            while ((RightIndex + 1) < Right.Length() && Right[RightIndex] == '0' && CharTraits::IsDigit(Right[RightIndex + 1]))
            {
                ++RightIndex;
            }

            int32 LeftEnd = LeftIndex;
            while (LeftEnd < Left.Length() && CharTraits::IsDigit(Left[LeftEnd]))
            {
                ++LeftEnd;
            }

            int32 RightEnd = RightIndex;
            while (RightEnd < Right.Length() && CharTraits::IsDigit(Right[RightEnd]))
            {
                ++RightEnd;
            }

            if ((LeftEnd - LeftIndex) != (RightEnd - RightIndex))
            {
                return (LeftEnd - LeftIndex) < (RightEnd - RightIndex) ? -1 : 1;
            }

            while (LeftIndex < LeftEnd && Left[LeftIndex] == Right[RightIndex])
            {
                ++LeftIndex;
                ++RightIndex;
            }

            if (LeftIndex < LeftEnd)
            {
                return Left[LeftIndex] < Right[RightIndex] ? -1 : 1;
            }

            continue;
        }

        if (Left[LeftIndex] != Right[RightIndex])
        {
            return Left[LeftIndex] < Right[RightIndex] ? -1 : 1;
        }

        ++LeftIndex;
        ++RightIndex;
    }

    const int32 LeftRemaining  = Left.Length() - LeftIndex;
    const int32 RightRemaining = Right.Length() - RightIndex;

    if (LeftRemaining != RightRemaining)
    {
        return LeftRemaining < RightRemaining ? -1 : 1;
    }

    return 0;
}

void FinalizeProfilerIntervals(TArray<FProfilerInterval>& Intervals, uint64 Frequency)
{
    if (Intervals.IsEmpty())
    {
        return;
    }

    Intervals.SortWithPredicate([](const FProfilerInterval& Left, const FProfilerInterval& Right)
    {
        if (Left.StartTimeStamp != Right.StartTimeStamp)
        {
            return Left.StartTimeStamp < Right.StartTimeStamp;
        }

        if (Left.Depth != Right.Depth)
        {
            return Left.Depth < Right.Depth;
        }

        return Left.EndTimeStamp > Right.EndTimeStamp;
    });

    TArray<int32> IndexAtDepth;
    for (int32 Index = 0; Index < Intervals.Size(); ++Index)
    {
        FProfilerInterval& Interval = Intervals[Index];
        const uint64 Delta = Interval.EndTimeStamp >= Interval.StartTimeStamp
            ? (Interval.EndTimeStamp - Interval.StartTimeStamp)
            : 0;

        Interval.InclusiveNanoseconds = ProfilerTicksToNanoseconds(Delta, Frequency);
        Interval.ExclusiveNanoseconds = Interval.InclusiveNanoseconds;

        const int32 Depth = Math::Max(Interval.Depth, 0);
        IndexAtDepth.Resize(Depth);

        Interval.ParentIndex = IndexAtDepth.IsEmpty() ? -1 : IndexAtDepth.Last();
        IndexAtDepth.Add(Index);
    }

    ApplyExclusiveTimesFromParentIndex(Intervals);
}

void SortProfilerThreadFrames(TArray<FProfilerThreadFrame>& Threads)
{
    if (Threads.Size() < 2)
    {
        return;
    }

    struct FThreadOrder
    {
        String Name;
        bool   bIsMainThread = false;
        bool   bIsNamed      = false;
    };

    FThreadManager& ThreadManager = FThreadManager::Get();

    TMap<void*, FThreadOrder> Order;
    for (const FProfilerThreadFrame& ThreadFrame : Threads)
    {
        FThreadOrder Entry;
        Entry.bIsMainThread = ThreadManager.IsMainThread(ThreadFrame.ThreadHandle);

        if (IPlatformThread* Thread = ThreadManager.FindThreadFromHandle(ThreadFrame.ThreadHandle))
        {
            Entry.Name     = Thread->GetName();
            Entry.bIsNamed = true;
        }

        Order.Add(ThreadFrame.ThreadHandle, Entry);
    }

    Threads.SortWithPredicate([&Order](const FProfilerThreadFrame& Left, const FProfilerThreadFrame& Right)
    {
        const FThreadOrder* LeftOrder  = Order.Find(Left.ThreadHandle);
        const FThreadOrder* RightOrder = Order.Find(Right.ThreadHandle);
        if (!LeftOrder || !RightOrder)
        {
            return false;
        }

        if (LeftOrder->bIsMainThread != RightOrder->bIsMainThread)
        {
            return LeftOrder->bIsMainThread;
        }

        if (LeftOrder->bIsNamed != RightOrder->bIsNamed)
        {
            return LeftOrder->bIsNamed;
        }

        if (LeftOrder->bIsNamed)
        {
            const int32 NameOrder = CompareThreadNames(LeftOrder->Name, RightOrder->Name);
            if (NameOrder != 0)
            {
                return NameOrder < 0;
            }
        }

        return reinterpret_cast<uintptr_t>(Left.ThreadHandle) < reinterpret_cast<uintptr_t>(Right.ThreadHandle);
    });
}

void CollectProfilerCallers(const TArray<FProfilerInterval>& Intervals, int32 Index, TArray<int32>& OutCallers)
{
    OutCallers.Clear();

    int32 Current = Index;
    while (Current >= 0 && Current < Intervals.Size())
    {
        const int32 ParentIndex = Intervals[Current].ParentIndex;
        if (ParentIndex < 0)
        {
            break;
        }

        OutCallers.Add(ParentIndex);
        Current = ParentIndex;
    }

    OutCallers.Reverse();
}
