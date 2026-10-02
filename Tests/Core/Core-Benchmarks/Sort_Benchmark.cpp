#include "Sort_Benchmark.h"

#if RUN_SORT_BENCHMARKS
#include "TestCommon/Benchmark.h"

#include <Core/Algorithms/Algorithm.h>
#include <Core/Containers/Array.h>
#include <Core/Math/Math.h>
#include <Core/Math/Random.h>
#include <Core/Math/Vector3.h>
#include <Core/Templates/CString.h>

#include <algorithm>
#include <vector>

struct FFatValue
{
    int32 Key = 0;
    uint8 Pad[64] = {};
};

static void FillRandom(TArray<int32>& Values, uint32 Count, uint32 Seed)
{
    FRandom Random(Seed);
    Values.ResizeUninitialized(static_cast<int32>(Count));
    for (uint32 Index = 0; Index < Count; ++Index)
    {
        Values[static_cast<int32>(Index)] = static_cast<int32>(Random.Rand());
    }
}

static void MakeDistribution(TArray<int32>& Values, const CHAR* Name)
{
    const int32 Count = Values.Size();
    if (TCString<CHAR>::Strcmp(Name, "sorted") == 0)
    {
        Algorithm::Sort(Values);
    }
    else if (TCString<CHAR>::Strcmp(Name, "reverse") == 0)
    {
        Algorithm::Sort(Values);
        Values.Reverse();
    }
    else if (TCString<CHAR>::Strcmp(Name, "nearly") == 0)
    {
        Algorithm::Sort(Values);
        const int32 Swaps = Math::Max(1, Count / 100);
        for (int32 Index = 0; Index < Swaps && Count > 1; ++Index)
        {
            const int32 A = Index % Count;
            const int32 B = (Index * 7) % Count;
            ::Swap(Values[A], Values[B]);
        }
    }
    else if (TCString<CHAR>::Strcmp(Name, "few-unique") == 0)
    {
        for (int32 Index = 0; Index < Count; ++Index)
        {
            Values[Index] = Values[Index] % 4;
        }
    }
    else if (TCString<CHAR>::Strcmp(Name, "equal") == 0)
    {
        for (int32 Index = 0; Index < Count; ++Index)
        {
            Values[Index] = 1;
        }
    }
    else if (TCString<CHAR>::Strcmp(Name, "organ-pipe") == 0)
    {
        Algorithm::Sort(Values);
        TArray<int32> Pipe;
        Pipe.Reserve(Count);
        for (int32 Index = 0; Index < Count / 2; ++Index)
        {
            Pipe.Add(Values[Index]);
        }
        for (int32 Index = Count / 2 - 1; Index >= 0; --Index)
        {
            Pipe.Add(Values[Index]);
        }
        while (Pipe.Size() < Count)
        {
            Pipe.Add(Values[0]);
        }
        Values = Pipe;
    }
}

template<typename SortFnType>
static void TimeSort(const CHAR* Label, const TArray<int32>& Source, uint32 Trials, SortFnType&& SortFn)
{
    FClock Clock;
    for (uint32 Trial = 0; Trial < Trials; ++Trial)
    {
        TArray<int32> Values = Source;
        FScopedClock ScopedClock(Clock);
        SortFn(Values);
    }

    Benchmark::Report(Label, Clock.GetTotalDuration() / static_cast<int64>(Trials));
}

void Sort_Benchmark()
{
    LOG_INFO("\n=== Sort Benchmarks ===");

    const uint32 SmallTrials = Benchmark::ScaleCount(200, 10);
    const uint32 LargeTrials = Benchmark::ScaleCount(20, 2);

    const int32 Sizes[] = { 32, 256, 4096, 65536, 1000000 };
    const CHAR* Distributions[] = { "random", "sorted", "reverse", "nearly", "few-unique", "equal", "organ-pipe" };

    for (int32 Size : Sizes)
    {
        const uint32 Trials = Size >= 65536 ? LargeTrials : SmallTrials;
        TArray<int32> Base;
        FillRandom(Base, static_cast<uint32>(Size), 0xC0FFEEu);

        for (const CHAR* Distribution : Distributions)
        {
            if (Size >= 1000000 && TCString<CHAR>::Strcmp(Distribution, "random") != 0
                && TCString<CHAR>::Strcmp(Distribution, "sorted") != 0
                && TCString<CHAR>::Strcmp(Distribution, "reverse") != 0)
            {
                continue;
            }

            TArray<int32> Source = Base;
            MakeDistribution(Source, Distribution);

            LOG_INFO("\nint32 n=%d dist=%s trials=%u", Size, Distribution, Trials);

            TimeSort("Algorithm::Sort", Source, Trials, [](TArray<int32>& Values)
            {
                Algorithm::Sort(Values);
            });
            TimeSort("TArray::Sort", Source, Trials, [](TArray<int32>& Values)
            {
                Values.Sort();
            });
            TimeSort("std::sort", Source, Trials, [](TArray<int32>& Values)
            {
                std::sort(Values.Data(), Values.Data() + Values.Size());
            });
            TimeSort("std::stable_sort", Source, Trials, [](TArray<int32>& Values)
            {
                std::stable_sort(Values.Data(), Values.Data() + Values.Size());
            });
            TimeSort("Algorithm::HeapSort", Source, Trials, [](TArray<int32>& Values)
            {
                Algorithm::HeapSort(Values);
            });
            TimeSort("std::heap", Source, Trials, [](TArray<int32>& Values)
            {
                std::make_heap(Values.Data(), Values.Data() + Values.Size());
                std::sort_heap(Values.Data(), Values.Data() + Values.Size());
            });
            TimeSort("Algorithm::IntegerSortBy", Source, Trials, [](TArray<int32>& Values)
            {
                Algorithm::IntegerSortBy(Values, [](int32 Value) { return Value; });
            });
            TimeSort("Algorithm::StableSort", Source, Trials, [](TArray<int32>& Values)
            {
                Algorithm::StableSort(Values);
            });
        }
    }

    {
        LOG_INFO("\nVector3 random n=65536");
        const uint32 Trials = LargeTrials;
        FRandom Random(99);
        TArray<Vector3> Source;
        Source.Reserve(65536);
        for (int32 Index = 0; Index < 65536; ++Index)
        {
            Source.Emplace(static_cast<float>(Random.Rand()), static_cast<float>(Random.Rand()), static_cast<float>(Random.Rand()));
        }

        {
            FClock Clock;
            for (uint32 Trial = 0; Trial < Trials; ++Trial)
            {
                TArray<Vector3> Values = Source;
                FScopedClock ScopedClock(Clock);
                Algorithm::Sort(Values, [](const Vector3& Left, const Vector3& Right)
                {
                    return Left.X < Right.X;
                });
            }
            Benchmark::Report("Algorithm::Sort Vector3", Clock.GetTotalDuration() / static_cast<int64>(Trials));
        }
        {
            FClock Clock;
            for (uint32 Trial = 0; Trial < Trials; ++Trial)
            {
                TArray<Vector3> Values = Source;
                FScopedClock ScopedClock(Clock);
                std::sort(Values.Data(), Values.Data() + Values.Size(), [](const Vector3& Left, const Vector3& Right)
                {
                    return Left.X < Right.X;
                });
            }
            Benchmark::Report("std::sort Vector3", Clock.GetTotalDuration() / static_cast<int64>(Trials));
        }
    }

    {
        LOG_INFO("\nFat struct random n=65536");
        const uint32 Trials = LargeTrials;
        FRandom Random(77);
        TArray<FFatValue> Source;
        Source.Reserve(65536);
        for (int32 Index = 0; Index < 65536; ++Index)
        {
            FFatValue Value;
            Value.Key = static_cast<int32>(Random.Rand());
            Source.Add(Value);
        }

        {
            FClock Clock;
            for (uint32 Trial = 0; Trial < Trials; ++Trial)
            {
                TArray<FFatValue> Values = Source;
                FScopedClock ScopedClock(Clock);
                Algorithm::Sort(Values, [](const FFatValue& Left, const FFatValue& Right)
                {
                    return Left.Key < Right.Key;
                });
            }
            Benchmark::Report("Algorithm::Sort fat", Clock.GetTotalDuration() / static_cast<int64>(Trials));
        }
        {
            FClock Clock;
            for (uint32 Trial = 0; Trial < Trials; ++Trial)
            {
                TArray<FFatValue> Values = Source;
                FScopedClock ScopedClock(Clock);
                std::sort(Values.Data(), Values.Data() + Values.Size(), [](const FFatValue& Left, const FFatValue& Right)
                {
                    return Left.Key < Right.Key;
                });
            }
            Benchmark::Report("std::sort fat", Clock.GetTotalDuration() / static_cast<int64>(Trials));
        }
    }
}

#endif
