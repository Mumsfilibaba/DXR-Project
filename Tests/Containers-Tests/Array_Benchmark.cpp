#include "Array_Test.h"

#if RUN_TARRAY_TEST || RUN_TARRAY_BENCHMARKS
#include "TestUtils.h"
#include "TestCommon/Benchmark.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Math/Random.h>
#include <Core/Math/Vector3.h>

#include <vector>
#include <algorithm>

#define ENABLE_INLINE_ALLOCATOR (1)
#define ENABLE_SHRINKTOFIT_BENCHMARK (0)
#define ENABLE_SORT_BENCHMARK (1)
#define ENABLE_STANDARD_BENCHMARK (1)

// Individual operations within the standard benchmark (apply to both String and Vector3).
#define ENABLE_INSERT_BENCHMARK    (1)
#define ENABLE_EMPLACEAT_BENCHMARK (1)
#define ENABLE_ADD_BENCHMARK       (1)
#define ENABLE_EMPLACE_BENCHMARK   (1)

// Heap-sort comparison within the sort benchmark.
#define ENABLE_HEAPSORT_BENCHMARK  (1)

#if ENABLE_INLINE_ALLOCATOR
template<typename T>
using TArrayAllocator = TInlineArrayAllocator<T, 1024>;
#else
template<typename T>
using TArrayAllocator = TDefaultArrayAllocator<T>;
#endif

#if RUN_TARRAY_BENCHMARKS

void TArray_Benchmark()
{
#if ENABLE_STANDARD_BENCHMARK
    LOG_INFO("\nBenchmark (String)");
    const uint32 TestCount = Benchmark::ScaleCount(100, 10);

    // Insert
#if ENABLE_INSERT_BENCHMARK
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nInsert/insert (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<String> Strings0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings0.insert(Strings0.begin(), "My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<String, TArrayAllocator<String>> Strings1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings1.Insert(0, "My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif

#if ENABLE_EMPLACEAT_BENCHMARK
    // EmplaceAt
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nEmplaceAt/emplace (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<String> Strings0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings0.emplace(Strings0.begin(), "My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<String, TArrayAllocator<String>> Strings1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings1.EmplaceAt(0, "My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif

#if ENABLE_ADD_BENCHMARK
    // Add
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nAdd/push_back (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<String> Strings0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings0.push_back("My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<String, TArrayAllocator<String>> Strings1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings1.Add("My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif

#if ENABLE_EMPLACE_BENCHMARK
    // Emplace
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nEmplace/emplace_back (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<String> Strings0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings0.emplace_back("My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<String, TArrayAllocator<String>> Strings1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings1.Emplace("My name is jeff");

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Strings1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif

    LOG_INFO("\nBenchmark (Vector3)");

    // Insert
#if ENABLE_INSERT_BENCHMARK
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nInsert/insert (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<Vector3> Vectors0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.insert(Vectors0.begin(), Vector3(3.0f, 5.0f, -6.0f));

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<Vector3, TArrayAllocator<Vector3>> Vectors1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.Insert(0, Vector3(3.0f, 5.0f, -6.0f));

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif

#if ENABLE_EMPLACEAT_BENCHMARK
    // EmplaceAt
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nEmplaceAt/emplace (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<Vector3> Vectors0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.emplace(Vectors0.begin(), 3.0f, 5.0f, -6.0f);

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<Vector3, TArrayAllocator<Vector3>> Vectors1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.EmplaceAt(0, float(j + 1), 5.0f, -6.0f);

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif

#if ENABLE_ADD_BENCHMARK
    // Add
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nAdd/push_back (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<Vector3> Vectors0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.push_back(Vector3(3.0f, 5.0f, -6.0f));

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<Vector3, TArrayAllocator<Vector3>> Vectors1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.Add(Vector3(3.0f, 5.0f, -6.0f));

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif

#if ENABLE_EMPLACE_BENCHMARK
    // Emplace
    {
        const uint32 Iterations = Benchmark::ScaleCount(10000, 1000);
        LOG_INFO("\nEmplace/emplace_back (Iterations=%u, TestCount=%u)", Iterations, TestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<Vector3> Vectors0;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.emplace_back(3.0f, 5.0f, -6.0f);

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors0.shrink_to_fit();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("std::vector", Duration, Iterations);
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<Vector3, TArrayAllocator<Vector3>> Vectors1;

                FScopedClock ScopedClock(Clock);

            #if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
            #endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.Emplace(3.0f, 5.0f, -6.0f);

                #if ENABLE_SHRINKTOFIT_BENCHMARK
                    if (ResetCounter >= 5)
                    {
                        Vectors1.Shrink();
                        ResetCounter = 0;
                    }

                    ++ResetCounter;
                #endif
                }
            }

            auto Duration = Clock.GetTotalDuration() / TestCount;
            Benchmark::Report("TArray", Duration, Iterations);
        }
    }
#endif
#endif

#if ENABLE_SORT_BENCHMARK
    {
        const uint32 SortTestCount = Benchmark::ScaleCount(100, 10);

        const uint32 NumNumbers = 1'000'000;

        FRandom Random;

        std::vector<int32> Numbers;
        Numbers.reserve(NumNumbers);
        for (uint32 n = 0; n < NumNumbers; n++)
        {
            Numbers.emplace_back(static_cast<int32>(Random.Rand()));
        }

    #if ENABLE_HEAPSORT_BENCHMARK
        LOG_INFO("\nHeapSort/std::heap_sort (NumNumbers=%u, SortTestCount=%u)", NumNumbers, SortTestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < SortTestCount; ++i)
            {
                TArray<int32, TArrayAllocator<int32>> Heap;
                for (uint32 n = 0; n < NumNumbers; n++)
                {
                    Heap.Emplace(Numbers[n]);
                }

                {
                    FScopedClock ScopedClock(Clock);
                    Heap.HeapSort();
                }
            }

            const auto Duration = Clock.GetTotalDuration() / SortTestCount;
            LOG_INFO("TArray      Sorting time: %lldns", static_cast<long long>(Duration));
            LOG_INFO("TArray      Sorting time: %lldms", static_cast<long long>(Duration / (1000 * 1000)));
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < SortTestCount; ++i)
            {
                std::vector<int32> Heap;
                for (uint32 n = 0; n < NumNumbers; n++)
                {
                    Heap.emplace_back(Numbers[n]);
                }

                {
                    FScopedClock ScopedClock(Clock);
                    std::make_heap(Heap.begin(), Heap.end());
                    std::sort_heap(Heap.begin(), Heap.end());
                }
            }

            const auto Duration = Clock.GetTotalDuration() / SortTestCount;
            LOG_INFO("std::vector Sorting time: %lldns", static_cast<long long>(Duration));
            LOG_INFO("std::vector Sorting time: %lldms", static_cast<long long>(Duration / (1000 * 1000)));
        }
    #endif // ENABLE_HEAPSORT_BENCHMARK

        LOG_INFO("\nSort/std::sort (NumNumbers=%u, SortTestCount=%u)", NumNumbers, SortTestCount);
        {
            FClock Clock;
            for (uint32 i = 0; i < SortTestCount; ++i)
            {
                TArray<int32, TArrayAllocator<int32>> Heap;
                for (uint32 n = 0; n < NumNumbers; n++)
                {
                    Heap.Emplace(Numbers[n]);
                }

                {
                    FScopedClock ScopedClock(Clock);
                    Heap.Sort();
                }
            }

            const auto Duration = Clock.GetTotalDuration() / SortTestCount;
            LOG_INFO("TArray      Sorting time: %lldns", static_cast<long long>(Duration));
            LOG_INFO("TArray      Sorting time: %lldms", static_cast<long long>(Duration / (1000 * 1000)));
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < SortTestCount; ++i)
            {
                std::vector<int32> Heap;
                for (uint32 n = 0; n < NumNumbers; n++)
                {
                    Heap.emplace_back(Numbers[n]);
                }

                {
                    FScopedClock ScopedClock(Clock);
                    std::sort(Heap.begin(), Heap.end());
                }
            }

            const auto Duration = Clock.GetTotalDuration() / SortTestCount;
            LOG_INFO("std::vector Sorting time: %lldns", static_cast<long long>(Duration));
            LOG_INFO("std::vector Sorting time: %lldms", static_cast<long long>(Duration / (1000 * 1000)));
        }
    }
#endif
}

#endif // RUN_TARRAY_BENCHMARKS

#endif // RUN_TARRAY_TEST || RUN_TARRAY_BENCHMARKS
