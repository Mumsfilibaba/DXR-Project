#include "Map_Benchmark.h"

#if RUN_TMAP_BENCHMARKS
#include "TestCommon/Benchmark.h"

#include <Core/Containers/Map.h>
#include <Core/Containers/MultiMap.h>
#include <Core/Containers/Set.h>
#include <Core/Containers/String.h>
#include <Core/Templates/TypeHash.h>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <type_traits>

static uint64 GStdAllocatedBytes = 0;

struct FStdHasher
{
    template<typename T>
    size_t operator()(const T& Value) const
    {
        return static_cast<size_t>(THash<T>::GetHash(Value));
    }
};

template<typename T>
struct TCountingAllocator
{
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template<typename U>
    struct rebind
    {
        using other = TCountingAllocator<U>;
    };

    TCountingAllocator() = default;

    template<typename U>
    TCountingAllocator(const TCountingAllocator<U>&)
    {
    }

    T* allocate(size_t Count)
    {
        const size_t Bytes = Count * sizeof(T);
        GStdAllocatedBytes += Bytes;
        return static_cast<T*>(::operator new(Bytes));
    }

    void deallocate(T* Pointer, size_t Count)
    {
        const size_t Bytes = Count * sizeof(T);
        if (GStdAllocatedBytes >= Bytes)
        {
            GStdAllocatedBytes -= Bytes;
        }

        ::operator delete(Pointer);
    }

    template<typename U>
    bool operator==(const TCountingAllocator<U>&) const
    {
        return true;
    }

    template<typename U>
    bool operator!=(const TCountingAllocator<U>&) const
    {
        return false;
    }
};

void TMap_Benchmark()
{
    const uint32 TestCount  = Benchmark::ScaleCount(20, 4);
    const uint32 Iterations = Benchmark::ScaleCount(20000, 2000);

    LOG_INFO("\nTMap / TSet / TMultiMap vs std (Iterations=%u, TestCount=%u)", Iterations, TestCount);

    {
        LOG_INFO("\nInsert int32");
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::unordered_map<int32, int32, FStdHasher> Map;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.emplace(static_cast<int32>(j), static_cast<int32>(j));
                }
            }

            Benchmark::Report("std::unordered_map", Clock.GetTotalDuration() / TestCount, Iterations);
        }

        {
            FClock Clock;
            TMap<int32, int32> Last;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TMap<int32, int32> Map;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.Add(static_cast<int32>(j), static_cast<int32>(j));
                }
                Last = Move(Map);
            }

            Benchmark::Report("TMap", Clock.GetTotalDuration() / TestCount, Iterations);
            LOG_INFO("TMap allocated               :%llu bytes (last run)", static_cast<unsigned long long>(Last.GetAllocatedSize()));
        }
    }

    {
        LOG_INFO("\nFind int32 (hits)");
        std::unordered_map<int32, int32, FStdHasher> StdMap;
        TMap<int32, int32> NativeMap;
        StdMap.reserve(Iterations);
        NativeMap.Reserve(static_cast<int32>(Iterations));
        for (uint32 j = 0; j < Iterations; ++j)
        {
            StdMap.emplace(static_cast<int32>(j), static_cast<int32>(j));
            NativeMap.Add(static_cast<int32>(j), static_cast<int32>(j));
        }

        {
            FClock Clock;
            volatile int32 Sink = 0;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Sink += StdMap.find(static_cast<int32>(j))->second;
                }
            }

            Benchmark::Report("std::unordered_map find", Clock.GetTotalDuration() / TestCount, Iterations);
        }

        {
            FClock Clock;
            volatile int32 Sink = 0;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Sink += *NativeMap.Find(static_cast<int32>(j));
                }
            }

            Benchmark::Report("TMap find", Clock.GetTotalDuration() / TestCount, Iterations);
        }
    }

    {
        LOG_INFO("\nErase int32");
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::unordered_map<int32, int32, FStdHasher> Map;
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.emplace(static_cast<int32>(j), static_cast<int32>(j));
                }

                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.erase(static_cast<int32>(j));
                }
            }

            Benchmark::Report("std::unordered_map erase", Clock.GetTotalDuration() / TestCount, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TMap<int32, int32> Map;
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.Add(static_cast<int32>(j), static_cast<int32>(j));
                }

                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.Remove(static_cast<int32>(j));
                }
            }

            Benchmark::Report("TMap erase", Clock.GetTotalDuration() / TestCount, Iterations);
        }
    }

    {
        LOG_INFO("\nIterate int32");
        std::unordered_map<int32, int32, FStdHasher> StdMap;
        TMap<int32, int32> NativeMap;
        for (uint32 j = 0; j < Iterations; ++j)
        {
            StdMap.emplace(static_cast<int32>(j), static_cast<int32>(j));
            NativeMap.Add(static_cast<int32>(j), static_cast<int32>(j));
        }

        {
            FClock Clock;
            volatile int32 Sink = 0;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                FScopedClock ScopedClock(Clock);
                for (const auto& Pair : StdMap)
                {
                    Sink += Pair.second;
                }
            }

            Benchmark::Report("std::unordered_map iter", Clock.GetTotalDuration() / TestCount, Iterations);
        }

        {
            FClock Clock;
            volatile int32 Sink = 0;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                FScopedClock ScopedClock(Clock);
                for (const auto& Pair : NativeMap)
                {
                    Sink += Pair.Second;
                }
            }

            Benchmark::Report("TMap iter", Clock.GetTotalDuration() / TestCount, Iterations);
        }
    }

    {
        LOG_INFO("\nTSet insert int32");
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::unordered_set<int32, FStdHasher> Set;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Set.emplace(static_cast<int32>(j));
                }
            }

            Benchmark::Report("std::unordered_set", Clock.GetTotalDuration() / TestCount, Iterations);
        }

        {
            FClock Clock;
            TSet<int32> Last;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TSet<int32> Set;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Set.Add(static_cast<int32>(j));
                }
                Last = Move(Set);
            }

            Benchmark::Report("TSet", Clock.GetTotalDuration() / TestCount, Iterations);
            LOG_INFO("TSet allocated               :%llu bytes (last run)", static_cast<unsigned long long>(Last.GetAllocatedSize()));
        }
    }

    {
        LOG_INFO("\nTMultiMap insert int32");
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::unordered_multimap<int32, int32, FStdHasher> Map;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.emplace(static_cast<int32>(j % 100), static_cast<int32>(j));
                }
            }

            Benchmark::Report("std::unordered_multimap", Clock.GetTotalDuration() / TestCount, Iterations);
        }

        {
            FClock Clock;
            TMultiMap<int32, int32> Last;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TMultiMap<int32, int32> Map;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.Add(static_cast<int32>(j % 100), static_cast<int32>(j));
                }
                Last = Move(Map);
            }

            Benchmark::Report("TMultiMap", Clock.GetTotalDuration() / TestCount, Iterations);
            LOG_INFO("TMultiMap allocated          :%llu bytes (last run)", static_cast<unsigned long long>(Last.GetAllocatedSize()));
        }
    }

    {
        LOG_INFO("\nInsert String keys");
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::unordered_map<String, int32, FStdHasher> Map;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.emplace(String::Printf("%u", j), static_cast<int32>(j));
                }
            }

            Benchmark::Report("std map String", Clock.GetTotalDuration() / TestCount, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TMap<String, int32> Map;
                FScopedClock ScopedClock(Clock);
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Map.Add(String::Printf("%u", j), static_cast<int32>(j));
                }
            }

            Benchmark::Report("TMap String", Clock.GetTotalDuration() / TestCount, Iterations);
        }
    }

    {
        GStdAllocatedBytes = 0;
        {
            std::unordered_map<int32, int32, FStdHasher, std::equal_to<int32>, TCountingAllocator<std::pair<const int32, int32>>> Counted;
            for (uint32 j = 0; j < Iterations; ++j)
            {
                Counted.emplace(static_cast<int32>(j), static_cast<int32>(j));
            }

            TMap<int32, int32> Native;
            Native.Reserve(static_cast<int32>(Iterations));
            for (uint32 j = 0; j < Iterations; ++j)
            {
                Native.Add(static_cast<int32>(j), static_cast<int32>(j));
            }

            LOG_INFO("\nAllocated bytes after %u int inserts", Iterations);
            LOG_INFO("std::unordered_map           :%llu bytes", static_cast<unsigned long long>(GStdAllocatedBytes));
            LOG_INFO("TMap                         :%llu bytes", static_cast<unsigned long long>(Native.GetAllocatedSize()));
        }
    }
}
#endif
