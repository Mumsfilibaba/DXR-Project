#include "Array_Test.h"

#if RUN_TARRAY_TEST || RUN_TARRAY_BENCHMARKS
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

#include "TestUtils.h"

#include <Core/Containers/Array.h>

#define ENABLE_INLINE_ALLOCATOR         (1)
#define ENABLE_STD_STRING_REALLOCATABLE (1)
#define ENABLE_SHRINKTOFIT_BENCHMARK    (0)
#define ENABLE_SORT_BENCHMARK           (0)

#if ENABLE_INLINE_ALLOCATOR
template<typename T>
using TArrayAllocator = TInlineArrayAllocator<T, 1024>;
#else
template<typename T>
using TArrayAllocator = TDefaultArrayAllocator<T>;
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FClock

struct FClock
{
    friend struct FScopedClock;

public:
    FClock()
        : Duration( 0 )
        , TotalDuration( 0 )
    { }

    inline void Reset()
    {
        Duration = 0;
        TotalDuration = 0;
    }

    inline int64 GetLastDuration() const
    {
        return Duration;
    }

    inline int64 GetTotalDuration() const
    {
        return TotalDuration;
    }

private:
    inline void AddDuration(int64 InDuration)
    {
        Duration = InDuration;
        TotalDuration += Duration;
    }

    int64 Duration = 0;
    int64 TotalDuration = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FScopedClock

struct FScopedClock
{
    FScopedClock(FClock& InParent)
        : Parent(InParent)
        , t0(std::chrono::high_resolution_clock::now())
        , t1()
    {
        t0 = std::chrono::high_resolution_clock::now();
    }

    ~FScopedClock()
    {
        t1 = std::chrono::high_resolution_clock::now();
        Parent.AddDuration(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    FClock& Parent;
    std::chrono::steady_clock::time_point t0;
    std::chrono::steady_clock::time_point t1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FVec3

struct FVec3
{
    FVec3() = default;

    FVec3(double InX, double InY, double InZ)
        : x(InX)
        , y(InY)
        , z(InZ)
    { }

    double x;
    double y;
    double z;

    operator std::string() const
    {
        return std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z);
    }

    bool operator==(const FVec3& Other) const
    {
        return (x == Other.x) && (y == Other.y) && (z == Other.z);
    }
};

#if ENABLE_STD_STRING_REALLOCATABLE
/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsReallocatable

template<>
struct TIsReallocatable<std::string>
{
    enum { Value = true };
};
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// PrintArr

template<typename T>
void PrintArr(const TArray<T, TArrayAllocator<T>>& Arr, const std::string& Name = "")
{
    std::cout << Name << '\n';
    std::cout << "--------------------------------" << '\n';

    for (auto i : Arr)
    {
        std::cout << (std::string)i << '\n';
    }

    std::cout << "Size: " << Arr.GetSize() << '\n';
    std::cout << "GetCapacity: " << Arr.GetCapacity() << '\n';

    std::cout << "--------------------------------" << '\n' << '\n';
}

template<>
void PrintArr<int32>(const TArray<int32, TArrayAllocator<int32>>& Arr, const std::string& Name)
{
    std::cout << Name << '\n';
    std::cout << "--------------------------------" << '\n';

    for (auto i : Arr)
    {
        std::cout << i << '\n';
    }

    std::cout << "Size: " << Arr.GetSize() << '\n';
    std::cout << "GetCapacity: " << Arr.GetCapacity() << '\n';

    std::cout << "--------------------------------" << '\n' << '\n';
}

template<typename T>
void PrintArr(const std::vector<T>& Arr, const std::string& Name = "")
{
    std::cout << Name << '\n';
    std::cout << "--------------------------------" << '\n';

    for (auto i : Arr)
    {
        std::cout << i << '\n';
    }

    std::cout << "Size: " << Arr.size() << '\n';
    std::cout << "GetCapacity: " << Arr.capacity() << '\n';

    std::cout << "--------------------------------" << '\n' << '\n';
}

#define PRINT_ARRAY(Arr) PrintArr(Arr, #Arr)

#if RUN_TARRAY_BENCHMARKS

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TArray_Benchmark

void TArray_Benchmark()
{
    // Performance
    std::cout << '\n' << "Benchmark (std::string)" << '\n';
#if defined(DEBUG_BUILD)
    const uint32 TestCount = 10;
#else
    const uint32 TestCount = 100;
#endif

    // Insert
#if 1
    {
    #if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
    #else
        const uint32 Iterations = 10000;
    #endif
        std::cout << '\n' << "Insert/insert (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<std::string> Strings0;

                FScopedClock FScopedClock(Clock);

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<std::string, TArrayAllocator<std::string>> Strings1;

                FScopedClock FScopedClock(Clock);

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

#if 1
    // EmplaceAt
    {
    #if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
    #else
        const uint32 Iterations = 10000;
    #endif
        std::cout << '\n' << "EmplaceAt/emplace (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<std::string> Strings0;

                FScopedClock FScopedClock(Clock);

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<std::string, TArrayAllocator<std::string>> Strings1;

                FScopedClock FScopedClock(Clock);

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

#if 1
    // Push
    {
#if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
#else
        const uint32 Iterations = 10000;
#endif
        std::cout << '\n' << "Push/push_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<std::string> Strings0;

                FScopedClock FScopedClock(Clock);

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<std::string, TArrayAllocator<std::string>> Strings1;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Strings1.Push("My name is jeff");

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

#if 1
    // Emplace
    {
#if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
#else
        const uint32 Iterations = 10000;
#endif
        std::cout << '\n' << "Emplace/emplace_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<std::string> Strings0;

                FScopedClock FScopedClock(Clock);

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<std::string, TArrayAllocator<std::string>> Strings1;

                FScopedClock FScopedClock(Clock);

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

    std::cout << '\n' << "Benchmark (FVec3)" << '\n';

    // Insert
#if 1
    {
#if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
#else
        const uint32 Iterations = 10000;
#endif
        std::cout << '\n' << "Insert/insert (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<FVec3> Vectors0;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.insert(Vectors0.begin(), FVec3(3.0, 5.0, -6.0));

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<FVec3, TArrayAllocator<FVec3>> Vectors1;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.Insert(0, FVec3(3.0, 5.0, -6.0));

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

#if 1
    // EmplaceAt
    {
#if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
#else
        const uint32 Iterations = 10000;
#endif
        std::cout << '\n' << "EmplaceAt/emplace (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<FVec3> Vectors0;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.emplace(Vectors0.begin(), 3.0, 5.0, -6.0);

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<FVec3, TArrayAllocator<FVec3>> Vectors1;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.EmplaceAt(0, double(j + 1), 5.0, -6.0);

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

#if 1
    // Push
    {
#if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
#else
        const uint32 Iterations = 10000;
#endif
        std::cout << '\n' << "Push/push_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<FVec3> Vectors0;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.push_back(FVec3(3.0, 5.0, -6.0));

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<FVec3, TArrayAllocator<FVec3>> Vectors1;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.Push(FVec3(3.0, 5.0, -6.0));

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

#if 1
    // Emplace
    {
#if defined(DEBUG_BUILD) && PLATFORM_WINDOWS
        const uint32 Iterations = 1000;
#else
        const uint32 Iterations = 10000;
#endif
        std::cout << '\n' << "Emplace/emplace_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << '\n';
        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                std::vector<FVec3> Vectors0;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors0.emplace_back(3.0, 5.0, -6.0);

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
            std::cout << "std::vector                :" << Duration << "ns" << '\n';
            std::cout << "std::vector (Per insertion):" << Duration / Iterations << "ns" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < TestCount; ++i)
            {
                TArray<FVec3, TArrayAllocator<FVec3>> Vectors1;

                FScopedClock FScopedClock(Clock);

#if ENABLE_SHRINKTOFIT_BENCHMARK
                int32 ResetCounter = 0;
#endif
                for (uint32 j = 0; j < Iterations; ++j)
                {
                    Vectors1.Emplace(3.0, 5.0, -6.0);

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
            std::cout << "TArray                     :" << Duration << "ns" << '\n';
            std::cout << "TArray (Per insertion)     :" << Duration / Iterations << "ns" << '\n';
        }
    }
#endif

#if ENABLE_SORT_BENCHMARK
    {
    #if defined(DEBUG_BUILD)
        const uint32 SortTestCount = 10;
    #else
        const uint32 SortTestCount = 100;
    #endif

        const uint32 NumNumbers = 1000000;
        std::cout << '\n' << "HeapSort/heap_sort (NumNumbers=" << NumNumbers << ", SortTestCount=" << SortTestCount << ")" << '\n';

        srand((unsigned int)time(0));

        {
            FClock Clock;
            for (uint32 i = 0; i < SortTestCount; ++i)
            {
                TArray<int32, TArrayAllocator<int32>> Heap;
                for (uint32 n = 0; n < NumNumbers; n++)
                {
                    Heap.Emplace(rand());
                }

                {
                    FScopedClock FScopedClock(Clock);
                    Heap.HeapSort();
                }
            }

            auto Duration = Clock.GetTotalDuration() / SortTestCount;
            std::cout << "TArray      Sorting time: " << Duration << "ns" << '\n';
            std::cout << "TArray      Sorting time: " << Duration / (1000 * 1000) << "ms" << '\n';
        }

        {
            FClock Clock;
            for (uint32 i = 0; i < SortTestCount; ++i)
            {
                std::vector<int32> Heap;
                for (uint32 n = 0; n < NumNumbers; n++)
                {
                    Heap.emplace_back(rand());
                }

                {
                    FScopedClock FScopedClock(Clock);
                    std::make_heap(Heap.begin(), Heap.end());
                    std::sort_heap(Heap.begin(), Heap.end());
                }
            }

            auto Duration = Clock.GetTotalDuration() / SortTestCount;
            std::cout << "std::vector Sorting time: " << Duration << "ns" << '\n';
            std::cout << "std::vector Sorting time: " << Duration / (1000 * 1000) << "ms" << '\n';
        }
    }
#endif
}

#endif // RUN_TARRAY_BENCHMARKS

#if RUN_TARRAY_TEST

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TArray_Test

bool TArray_Test(int32 Argc, const CHAR** Argv)
{
#if 1
    std::cout << '\n' << "----------TArray----------" << '\n' << '\n';
    {
        std::string ArgvStr = Argc > 0 ? Argv[0] : "Filepath";

        std::cout << "Testing TArray<std::string>" << '\n';

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Testing Constructors

        std::cout << '\n' << "Testing Constructors:" << '\n' << '\n';

        TArray<std::string, TArrayAllocator<std::string>> Strings0;
        TEST_CHECK(Strings0.IsEmpty());

        TArray<std::string, TArrayAllocator<std::string>> Strings1(5, "Hello");
        TEST_CHECK_ARRAY(Strings1, { "Hello", "Hello", "Hello", "Hello", "Hello" });

        TArray<std::string, TArrayAllocator<std::string>> Strings2(Strings1.GetData(), Strings1.GetSize());
        TEST_CHECK_ARRAY(Strings2, { "Hello", "Hello", "Hello", "Hello", "Hello" });

        TArray<std::string, TArrayAllocator<std::string>> Strings3 =
        {
            "Hello World",
            "TArray",
            "This is a longer teststring"
        };

        TEST_CHECK_ARRAY(Strings3, { "Hello World", "TArray", "This is a longer teststring" });

        {
            /*///////////////////////////////////////////////////////////////////////////////////////////////*/
            // Testing Copy Constructor

            std::cout << '\n' << "Test Copy Constructor" << '\n' << '\n';

            TArray<std::string, TArrayAllocator<std::string>> Strings4 = Strings0;
            TEST_CHECK(Strings4.IsEmpty());
            
            TArray<std::string, TArrayAllocator<std::string>> Strings5 = Strings1;

            std::cout << '\n' << "Before move" << '\n' << '\n';

            TEST_CHECK(!Strings5.IsEmpty());
            TEST_CHECK_ARRAY(Strings5, { "Hello", "Hello", "Hello", "Hello", "Hello" });

            /*///////////////////////////////////////////////////////////////////////////////////////////////*/
            // Testing Move Constructor

            std::cout << "Test Move Constructor" << '\n' << '\n';

            TArray<std::string, TArrayAllocator<std::string>> Strings6 = Move(Strings5);

            std::cout << '\n' << "After move" << '\n' << '\n';

            TEST_CHECK(Strings5.IsEmpty());
            TEST_CHECK_ARRAY(Strings6, { "Hello", "Hello", "Hello", "Hello", "Hello" });
        }

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Testing Reset
        
        std::cout << '\n' << "Testing Reset" << '\n' << '\n';

        Strings0.Reset(7, "This is a teststring");
        TEST_CHECK_ARRAY(Strings0,
        {
            "This is a teststring",
            "This is a teststring",
            "This is a teststring",
            "This is a teststring",
            "This is a teststring",
            "This is a teststring",
            "This is a teststring"
        });

        Strings1.Reset({ "Test-String #1", "Test-String #2", "Test-String #3" });
        TEST_CHECK_ARRAY(Strings1, { "Test-String #1", "Test-String #2", "Test-String #3" });

        Strings2.Reset(Strings3.GetData(), Strings3.GetSize());
        TEST_CHECK_ARRAY(Strings2, { "Hello World", "TArray", "This is a longer teststring" });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Testing Resize

        std::cout << '\n' << "Testing Resize" << '\n' << '\n';

        TArray<std::string, TArrayAllocator<std::string>> Strings4;

        Strings4.Resize(10, "New String");
        TEST_CHECK_ARRAY(Strings4,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String"
        });

        Strings3.Resize(4, "Hi, hi");
        TEST_CHECK(Strings3.GetSize()     == 4);
        TEST_CHECK(Strings3.GetCapacity() == 4);
        TEST_CHECK_ARRAY(Strings3,
        {
            "Hello World",
            "TArray",
            "This is a longer teststring",
            "Hi, hi"
        });

        Strings1.Resize(6, "Hello World");
        TEST_CHECK(Strings1.GetSize()     == 6);
        TEST_CHECK(Strings1.GetCapacity() == 6);
        TEST_CHECK_ARRAY(Strings1, 
        { 
            "Test-String #1",
            "Test-String #2",
            "Test-String #3",
            "Hello World",
            "Hello World",
            "Hello World"
        });

        Strings3.Resize(5, "No i am your father");
        TEST_CHECK(Strings3.GetSize()     == 5);
        TEST_CHECK(Strings3.GetCapacity() == 5);
        TEST_CHECK_ARRAY(Strings3,
        {
            "Hello World",
            "TArray",
            "This is a longer teststring",
            "Hi, hi",
            "No i am your father"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Testing Shrinking Resize

        std::cout << "Testing Shrinking Resize" << '\n' << '\n';
        
        Strings4.Resize(2, "New String");
        TEST_CHECK_ARRAY(Strings4, { "New String", "New String" });

        std::cout << "Testing Growing Resize" << '\n' << '\n';

        Strings4.Resize(15, "New String");
        TEST_CHECK_ARRAY(Strings4,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Testing Reserve

        std::cout << '\n' << "Testing Reserve" << '\n' << '\n';

        Strings4.Reserve(Strings4.GetCapacity());
        TEST_CHECK(Strings4.GetSize()     == 15);
        TEST_CHECK(Strings4.GetCapacity() == 15);
        TEST_CHECK_ARRAY(Strings4,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "New String"
        });

        std::cout << "Shrinking" << '\n' << '\n';

        Strings4.Reserve(5);
        TEST_CHECK(Strings4.GetSize()     == 5);
        TEST_CHECK(Strings4.GetCapacity() == 5);
        TEST_CHECK_ARRAY(Strings4,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String"
        });

        std::cout << "Growing" << '\n' << '\n';
        
        Strings4.Reserve(10);
        TEST_CHECK(Strings4.GetSize()     == 5);
        TEST_CHECK(Strings4.GetCapacity() == 10);
        TEST_CHECK_ARRAY(Strings4,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String"
        });

        std::cout << "Resize" << '\n' << '\n';

        Strings4.Resize(Strings4.GetCapacity() - 2, "This spot is reserved");
        TEST_CHECK(Strings4.GetSize()     == 8);
        TEST_CHECK(Strings4.GetCapacity() == 10);
        TEST_CHECK_ARRAY(Strings4,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "This spot is reserved",
            "This spot is reserved",
            "This spot is reserved"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Shrink To Fit

        std::cout << '\n' << "Testing Shrink" << '\n' << '\n';

        Strings4.Shrink();
        TEST_CHECK(Strings4.GetSize()     == 8);
        TEST_CHECK(Strings4.GetCapacity() == 8);
        TEST_CHECK_ARRAY(Strings4,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "This spot is reserved",
            "This spot is reserved",
            "This spot is reserved"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Assignment

        std::cout << '\n' << "Testing Assignment" << '\n' << '\n';

        Strings0 = Strings4;
        TEST_CHECK(Strings0.GetSize()     == 8);
        TEST_CHECK(Strings0.GetCapacity() == 8);
        TEST_CHECK_ARRAY(Strings0,
        {
            "New String",
            "New String",
            "New String",
            "New String",
            "New String",
            "This spot is reserved",
            "This spot is reserved",
            "This spot is reserved"
        });

        Strings1 = Move(Strings3);
        TEST_CHECK(Strings1.GetSize()     == 5);
        TEST_CHECK(Strings1.GetCapacity() == 5);
        TEST_CHECK_ARRAY(Strings1,
        {
            "Hello World",
            "TArray",
            "This is a longer teststring",
            "Hi, hi",
            "No i am your father"
        });

        Strings2 =
        {
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string"
        };

        TEST_CHECK(Strings2.GetSize()     == 3);
        TEST_CHECK(Strings2.GetCapacity() == 5);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // PushBack

        std::cout << '\n' << "Testing PushBack" << '\n' << '\n';
        
        for (uint32 i = 0; i < 6; ++i)
        {
            Strings2.Push("This is Pushed String #" + std::to_string(i));
        }

        TEST_CHECK(Strings2.GetSize() == 9);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5"
        });

        std::cout << '\n' << "Testing PushBack" << '\n' << '\n';
        for (uint32 i = 0; i < 6; ++i)
        {
            Strings2.Push(ArgvStr);
        }

        PRINT_ARRAY(Strings2);

        TEST_CHECK(Strings2.GetSize() == 15);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(), 
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str()
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // EmplaceBack
        
        std::cout << '\n' << "Testing EmplaceBack" << '\n' << '\n';
        for (uint32 i = 0; i < 6; ++i)
        {
            Strings2.Emplace("This is an Emplaced String #" + std::to_string(i));
        }

        TEST_CHECK(Strings2.GetSize() == 21);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "This is an Emplaced String #3",
            "This is an Emplaced String #4",
            "This is an Emplaced String #5"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // PopBack

        std::cout << '\n' << "Testing PopBack" << '\n' << '\n';
        for (uint32 i = 0; i < 3; ++i)
        {
            Strings2.Pop();
        }

        PRINT_ARRAY(Strings2);
        
        TEST_CHECK(Strings2.GetSize() == 18);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // Insert

        std::cout << '\n' << "Testing Insert" << '\n' << '\n';

        std::cout << '\n' << "At front" << '\n' << '\n';
        Strings2.Insert(0, ArgvStr);

        PRINT_ARRAY(Strings2);

        TEST_CHECK(Strings2.GetSize() == 19);
        TEST_CHECK_ARRAY(Strings2,
        {
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2"
        });

        Strings2.Insert(0, "Inserted String");
        TEST_CHECK(Strings2.GetSize() == 20);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2"
        });

        Strings2.Insert(0, { "Inserted String #1", "Inserted String #2" });
        TEST_CHECK(Strings2.GetSize() == 22);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2"
        });

        std::cout << '\n' << "At Arbitrary" << '\n' << '\n';
        Strings2.Insert(2, ArgvStr);
        PRINT_ARRAY(Strings2);

        TEST_CHECK(Strings2.GetSize() == 23);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String #1",
            "Inserted String #2",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2"
        });

        Strings2.Insert(2, "Inserted String Again");
        TEST_CHECK(Strings2.GetSize() == 24);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2"
        });

        Strings2.Insert(2, { "Inserted String Again #1", "Inserted String Again #2" });
        TEST_CHECK(Strings2.GetSize() == 26);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1", 
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2"
        });

        std::cout << '\n' << "At End" << '\n' << '\n';

        Strings2.Insert(Strings2.GetSize(), { "Inserted String At End #1", "Inserted String At End #2" });
        TEST_CHECK(Strings2.GetSize() == 28);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1", 
            "Inserted String At End #2"
        });

        std::cout << '\n' << "At front after reallocation" << '\n' << '\n';

        // Add a shrink to fit to force reallocation
        Strings2.Shrink();
        TEST_CHECK(Strings2.GetCapacity() == 28);

        Strings2.Insert(0, ArgvStr);
        
        PRINT_ARRAY(Strings2);

        TEST_CHECK(Strings2.GetSize() == 29);
        TEST_CHECK_ARRAY(Strings2,
        {
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2"
        });

        // Add a shrink to fit to force reallocation
        Strings2.Shrink();
        TEST_CHECK(Strings2.GetCapacity() == 29);

        Strings2.Insert(0, "Inserted String Reallocated");
        TEST_CHECK(Strings2.GetSize() == 30);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2"
        });

        // Add a shrink to fit to force reallocation
        Strings2.Shrink();
        TEST_CHECK(Strings2.GetCapacity() == 30);

        Strings2.Insert(0, { "Inserted String Reallocated #1", "Inserted String Reallocated #2" });
        TEST_CHECK(Strings2.GetSize() == 32);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated #1",
            "Inserted String Reallocated #2",
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2"
        });

        std::cout << '\n' << "At Arbitrary after reallocation" << '\n' << '\n';

        // Add a shrink to fit to force reallocation
        Strings2.Shrink();
        TEST_CHECK(Strings2.GetCapacity() == 32);

        Strings2.Insert(2, ArgvStr);
        PRINT_ARRAY(Strings2);

        TEST_CHECK(Strings2.GetSize() == 33);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated #1",
            "Inserted String Reallocated #2",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2"
        });

        // Add a shrink to fit to force reallocation
        Strings2.Shrink();
        TEST_CHECK(Strings2.GetCapacity() == 33);

        Strings2.Insert(2, "Inserted String Again Reallocated");
        TEST_CHECK(Strings2.GetSize() == 34);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated #1",
            "Inserted String Reallocated #2",
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2"
        });

        // Add a shrink to fit to force reallocation
        Strings2.Shrink();
        TEST_CHECK(Strings2.GetCapacity() == 34);

        Strings2.Insert(2, { "Inserted String Again Reallocated #1", "Inserted String Again Reallocated #2" });
        TEST_CHECK(Strings2.GetSize() == 36);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated #1",
            "Inserted String Reallocated #2",
            "Inserted String Again Reallocated #1",
            "Inserted String Again Reallocated #2",
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2"
        });

        std::cout << '\n' << "At End after reallocation" << '\n' << '\n';

        // Add a shrink to fit to force reallocation
        Strings2.Shrink();
        TEST_CHECK(Strings2.GetCapacity() == 36);

        Strings2.Insert(Strings2.GetSize(), { "Inserted String At End Reallocated #1", "Inserted String At End Reallocated #2" });
        TEST_CHECK(Strings2.GetSize() == 38);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated #1",
            "Inserted String Reallocated #2",
            "Inserted String Again Reallocated #1",
            "Inserted String Again Reallocated #2",
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2",
            "Inserted String At End Reallocated #1", 
            "Inserted String At End Reallocated #2"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // RemoveAt

        std::cout << '\n' << "Testing RemoveAt" << '\n' << '\n';

        std::cout << '\n' << "At front" << '\n' << '\n';

        Strings2.RemoveAt(0);
        TEST_CHECK(Strings2.GetSize() == 37);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated #2",
            "Inserted String Again Reallocated #1",
            "Inserted String Again Reallocated #2",
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2",
            "Inserted String At End Reallocated #1",
            "Inserted String At End Reallocated #2"
        });

        std::cout << '\n' << "At Arbitrary" << '\n' << '\n';

        Strings2.RemoveAt(2);
        TEST_CHECK(Strings2.GetSize() == 36);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Reallocated #2",
            "Inserted String Again Reallocated #1",
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2",
            "Inserted String At End Reallocated #1",
            "Inserted String At End Reallocated #2"
        });

        /*///////////////////////////////////////////////////////////////////////////////////////////////*/
        // RemoveRangeAt

        std::cout << '\n' << "Testing RemoveRangeAt" << '\n' << '\n';

        std::cout << '\n' << "Range At front" << '\n' << '\n';
        
        Strings2.RemoveRangeAt(0, 2);
        TEST_CHECK(Strings2.GetSize() == 34);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String #1",
            "Inserted String #2",
            "Inserted String Again #1",
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2",
            "Inserted String At End Reallocated #1",
            "Inserted String At End Reallocated #2"
        });

        std::cout << '\n' << "Range At Arbitrary" << '\n' << '\n';

        Strings2.RemoveRangeAt(4, 3);
        TEST_CHECK(Strings2.GetSize() == 31);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
            "Inserted String At End #2",
            "Inserted String At End Reallocated #1",
            "Inserted String At End Reallocated #2"
        });

        std::cout << '\n' << "Range At End" << '\n' << '\n';
        
        Strings2.RemoveRangeAt(Strings2.GetSize() - 3, 3);
        TEST_CHECK(Strings2.GetSize() == 28);
        TEST_CHECK_ARRAY(Strings2,
        {
            "Inserted String Again Reallocated",
            ArgvStr.c_str(),
            "Inserted String Reallocated",
            ArgvStr.c_str(),
            "Inserted String Again #2",
            "Inserted String Again",
            ArgvStr.c_str(),
            "Inserted String",
            ArgvStr.c_str(),
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string",
            "This is Pushed String #0",
            "This is Pushed String #1",
            "This is Pushed String #2",
            "This is Pushed String #3",
            "This is Pushed String #4",
            "This is Pushed String #5",
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            ArgvStr.c_str(),
            "This is an Emplaced String #0",
            "This is an Emplaced String #1",
            "This is an Emplaced String #2",
            "Inserted String At End #1",
        });

        std::cout << '\n' << "Testing Erase In Loop" << '\n' << '\n';

        TArray<std::string, TArrayAllocator<std::string>> LoopStrings =
        {
            "Str0",
            "Str1",
            "Str2",
            "Str3",
            "Str4",
            "Str5",
        };

        std::cout << "Before" << '\n' << '\n';
        PRINT_ARRAY(LoopStrings);

        uint32 Index = 0;
        for (TArray<std::string, TArrayAllocator<std::string>>::IteratorType It = LoopStrings.StartIterator(); It != LoopStrings.EndIterator();)
        {
            if (Index > 1)
            {
                It = LoopStrings.RemoveAt(It);
            }
            else
            {
                It++;
            }

            Index++;
        }

        std::cout << "After" << '\n' << '\n';
        PRINT_ARRAY(LoopStrings);

        // Swap
        std::cout << '\n' << "Testing Swap" << '\n' << '\n';
        std::cout << "Before" << '\n' << '\n';
        PRINT_ARRAY(Strings0);
        PRINT_ARRAY(Strings2);

        Strings0.Swap(Strings2);

        std::cout << "After" << '\n' << '\n';
        PRINT_ARRAY(Strings0);
        PRINT_ARRAY(Strings2);

        // Iterators
        std::cout << '\n' << "Testing Iterators" << '\n';

        std::cout << '\n' << "Iterators" << '\n' << '\n';
        for (auto It = Strings2.StartIterator(); It != Strings2.EndIterator(); It++)
        {
            std::cout << (*It) << '\n';
        }

        TArray<std::string, TArrayAllocator<std::string>> EmptyArray;
        for (auto It = EmptyArray.StartIterator(); It != EmptyArray.EndIterator(); It++)
        {
            std::cout << (*It) << '\n';
        }

        std::cout << '\n';

        for (std::string& Str : Strings2)
        {
            std::cout << Str << '\n';
        }

        std::cout << '\n' << "Reverse Iterators" << '\n' << '\n';
        for (auto It = Strings2.ReverseStartIterator(); It != Strings2.ReverseEndIterator(); It++)
        {
            std::cout << (*It) << '\n';
        }

        // Fill 
        std::cout << '\n' << "Testing Fill" << '\n';
        Strings2.Fill("Fill Value");
        PRINT_ARRAY(Strings2);

        // Append
        std::cout << '\n' << "Testing Append" << '\n';
        Strings2.Append({ "Append #1", "Append #2", "Append #3", "Append #4", "Append #5", "Append #6" });
        PRINT_ARRAY(Strings2);

        // PopBackRange
        std::cout << '\n' << "Testing PopBackRange" << '\n';
        Strings2.PopRange(3);
        PRINT_ARRAY(Strings2);

        std::cout << '\n' << "Testing Equal operator" << '\n';
        TArray<std::string, TArrayAllocator<std::string>> EqualArray = Strings2;
        std::cout << "operator==" << std::boolalpha << (EqualArray == Strings2) << '\n';
    }
#endif

    // TArray<FVec3>
#if 1
    {
        std::cout << '\n' << "Testing TArray<FVec3>" << '\n';
        std::cout << '\n' << "Testing Constructors" << '\n';
        // Test constructors
        TArray<FVec3, TArrayAllocator<FVec3>> Vectors0;
        TArray<FVec3, TArrayAllocator<FVec3>> Vectors1(5, FVec3(1.0, 1.0, 1.0));
        TArray<FVec3, TArrayAllocator<FVec3>> Vectors2(Vectors1.GetData(), Vectors1.GetSize());
        TArray<FVec3, TArrayAllocator<FVec3>> Vectors3 =
        {
            FVec3(1.0, 1.0, 1.0),
            FVec3(2.0, 2.0, 2.0),
            FVec3(3.0, 3.0, 3.0)
        };

        PRINT_ARRAY(Vectors0);
        PRINT_ARRAY(Vectors1);
        PRINT_ARRAY(Vectors2);
        PRINT_ARRAY(Vectors3);

        {
            // Test copy an empty array
            TArray<FVec3, TArrayAllocator<FVec3>> Vectors4 = Vectors0;
            // Test copy an array with data
            TArray<FVec3, TArrayAllocator<FVec3>> Vectors5 = Vectors1;

            PRINT_ARRAY(Vectors4);

            std::cout << "Before move" << '\n' << '\n';
            PRINT_ARRAY(Vectors5);

            // Test move an array with data
            TArray<FVec3, TArrayAllocator<FVec3>> Vectors6 = Move(Vectors5);

            std::cout << "After move" << '\n' << '\n';
            PRINT_ARRAY(Vectors5);
            PRINT_ARRAY(Vectors6);
        }

        // Assign
        std::cout << '\n' << "Testing Assign" << '\n';
        Vectors0.Reset(7, FVec3(5.0, 5.0, 5.0));
        PRINT_ARRAY(Vectors0);

        Vectors1.Reset({ FVec3(1.0, 5.0, 5.0), FVec3(2.0, 5.0, 5.0), FVec3(3.0, 5.0, 5.0) });
        PRINT_ARRAY(Vectors1);

        Vectors2.Reset(Vectors3.GetData(), Vectors3.GetSize());
        PRINT_ARRAY(Vectors2);

        // Resize
        std::cout << '\n' << "Testing Resize" << '\n' << '\n';

        // Constructing a empty Array to test resize
        TArray<FVec3, TArrayAllocator<FVec3>> Vectors4;

        std::cout << "Before Resize" << '\n' << '\n';
        PRINT_ARRAY(Vectors4);
        PRINT_ARRAY(Vectors3);
        PRINT_ARRAY(Vectors1);

        Vectors4.Resize(10, FVec3(-10.0, -10.0, -10.0));
        Vectors3.Resize(0);
        Vectors1.Resize(6, FVec3(-5.0, 10.0, -15.0));

        std::cout << "After Resize" << '\n' << '\n';
        PRINT_ARRAY(Vectors4);
        PRINT_ARRAY(Vectors3);
        PRINT_ARRAY(Vectors1);

        std::cout << "Testing Shrinking Resize" << '\n' << '\n';
        Vectors4.Resize(2, FVec3(-15.0, -15.0, -15.0));
        PRINT_ARRAY(Vectors4);

        Vectors4.Resize(15, FVec3(23.0, 23.0, 23.0));
        PRINT_ARRAY(Vectors4);

        // Reserve
        std::cout << '\n' << "Testing Reserve" << '\n' << '\n';

        std::cout << "Before Reserve" << '\n' << '\n';
        PRINT_ARRAY(Vectors4);

        std::cout << "After Reserve" << '\n' << '\n';
        Vectors4.Reserve(Vectors4.GetCapacity());
        PRINT_ARRAY(Vectors4);

        std::cout << "Shrinking" << '\n' << '\n';
        Vectors4.Reserve(5);
        PRINT_ARRAY(Vectors4);

        std::cout << "Growing" << '\n' << '\n';
        Vectors4.Reserve(10);
        PRINT_ARRAY(Vectors4);

        std::cout << "Resize" << '\n' << '\n';
        Vectors4.Resize(Vectors4.GetCapacity() - 2, FVec3(-1.0f, -1.0f, -1.0f));
        PRINT_ARRAY(Vectors4);

        // Shrink To Fit
        std::cout << '\n' << "Testing Shrink" << '\n';

        std::cout << '\n' << "Before Shrink" << '\n' << '\n';
        PRINT_ARRAY(Vectors4);

        Vectors4.Shrink();

        std::cout << '\n' << "After Shrink" << '\n' << '\n';
        PRINT_ARRAY(Vectors4);

        // Assignment
        std::cout << '\n' << "Testing Assignment" << '\n' << '\n';

        Vectors3.Resize(3, FVec3(42.0, 42.0, 42.0));

        std::cout << "Before Assignment" << '\n' << '\n';
        PRINT_ARRAY(Vectors0);
        PRINT_ARRAY(Vectors1);
        PRINT_ARRAY(Vectors2);
        PRINT_ARRAY(Vectors3);
        PRINT_ARRAY(Vectors4);

        std::cout << "Vectors0 = Vectors4" << '\n';
        Vectors0 = Vectors4;

        std::cout << "Vectors1 = Move(Vectors3)" << '\n';
        Vectors1 = Move(Vectors3);

        std::cout << "Vectors2 = InitializerList" << '\n' << '\n';
        Vectors2 =
        {
            FVec3(9.0, 9.0, 9.0),
            FVec3(10.0, 10.0, 10.0),
            FVec3(11.0, 11.0, 11.0)
        };

        std::cout << "After Assignment" << '\n' << '\n';
        PRINT_ARRAY(Vectors0);
        PRINT_ARRAY(Vectors1);
        PRINT_ARRAY(Vectors2);
        PRINT_ARRAY(Vectors3);
        PRINT_ARRAY(Vectors4);

        // PushBack
        std::cout << '\n' << "Testing PushBack" << '\n' << '\n';
        for (uint32 i = 0; i < 6; ++i)
        {
            Vectors2.Push(FVec3(7.0, 7.0, 7.0));
        }
        PRINT_ARRAY(Vectors2);

        FVec3 Vector(5.0f, -45.0f, 5.0f);
        std::cout << '\n' << "Testing PushBack" << '\n' << '\n';
        for (uint32 i = 0; i < 6; ++i)
        {
            Vectors2.Push(Vector);
        }
        PRINT_ARRAY(Vectors2);

        // EmplaceBack
        std::cout << '\n' << "Testing EmplaceBack" << '\n' << '\n';
        for (uint32 i = 0; i < 6; ++i)
        {
            Vectors2.Emplace(1.0, 0.0, 1.0);
        }
        PRINT_ARRAY(Vectors2);

        // PopBack
        std::cout << '\n' << "Testing PopBack" << '\n' << '\n';
        for (uint32 i = 0; i < 3; ++i)
        {
            Vectors2.Pop();
        }
        PRINT_ARRAY(Vectors2);

        // Insert
        std::cout << '\n' << "Testing Insert" << '\n' << '\n';
        std::cout << "At front" << '\n' << '\n';
        Vectors2.Insert(0, Vector);
        Vectors2.Insert(0, FVec3(-1.0, -1.0, -1.0));
        Vectors2.Insert(0, { FVec3(1.0f, 1.0f, 1.0f), FVec3(2.0f, 2.0f, 2.0f) });
        PRINT_ARRAY(Vectors2);

        std::cout << "At Arbitrary" << '\n' << '\n';
        Vectors2.Insert(2, Vector);
        Vectors2.Insert(2, FVec3(-1.0, -1.0, -2.0));
        Vectors2.Insert(2, { FVec3(1.0f, 1.0f, 2.0f), FVec3(2.0f, 2.0f, 3.0f) });
        PRINT_ARRAY(Vectors2);

        std::cout << "At End" << '\n' << '\n';
        Vectors2.Insert(Vectors2.GetSize(), { FVec3(1.0f, 1.0f, 3.0f), FVec3(2.0f, 2.0f, 4.0f) });
        PRINT_ARRAY(Vectors2);

        std::cout << "At front after reallocation" << '\n' << '\n';
        // Add a shrink to fit to force reallocation
        Vectors2.Shrink();
        Vectors2.Insert(0, Vector);
        // Add a shrink to fit to force reallocation
        Vectors2.Shrink();
        Vectors2.Insert(0, FVec3(-1.0, -1.0, -3.0));
        // Add a shrink to fit to force reallocation
        Vectors2.Shrink();
        Vectors2.Insert(0, { FVec3(1.0f, 1.0f, 4.0f), FVec3(2.0f, 2.0f, 5.0f) });
        PRINT_ARRAY(Vectors2);

        std::cout << "At Arbitrary after reallocation" << '\n' << '\n';
        // Add a shrink to fit to force reallocation
        Vectors2.Shrink();
        Vectors2.Insert(2, Vector);
        // Add a shrink to fit to force reallocation
        Vectors2.Shrink();
        Vectors2.Insert(2, FVec3(-1.0, -1.0, -4.0));
        // Add a shrink to fit to force reallocation
        Vectors2.Shrink();
        Vectors2.Insert(2, { FVec3(1.0f, 1.0f, 5.0f), FVec3(2.0f, 2.0f, 6.0f) });
        PRINT_ARRAY(Vectors2);

        std::cout << "At End after reallocation" << '\n' << '\n';
        // Add a shrink to fit to force reallocation
        Vectors2.Shrink();
        Vectors2.Insert(Vectors2.GetSize(), { FVec3(6.0f, 6.0f, 6.0f), FVec3(2.0f, 2.0f, 7.0f) });
        PRINT_ARRAY(Vectors2);

        // Erase
        std::cout << '\n' << "Testing Erase" << '\n' << '\n';
        PRINT_ARRAY(Vectors2);

        std::cout << "At front" << '\n' << '\n';
        Vectors2.RemoveAt(0);
        PRINT_ARRAY(Vectors2);

        std::cout << "At Arbitrary" << '\n' << '\n';
        Vectors2.RemoveAt(2);
        PRINT_ARRAY(Vectors2);

        std::cout << "Range At front" << '\n' << '\n';
        Vectors2.RemoveRangeAt(0, 2);
        PRINT_ARRAY(Vectors2);

        std::cout << "Range At Arbitrary" << '\n' << '\n';
        Vectors2.RemoveRangeAt(4, 3);
        PRINT_ARRAY(Vectors2);

        std::cout << "Range At End" << '\n' << '\n';
        Vectors2.RemoveRangeAt(Vectors2.GetSize() - 3, 3);
        PRINT_ARRAY(Vectors2);

        // Swap
        std::cout << '\n' << "Testing Swap" << '\n' << '\n';
        std::cout << "Before" << '\n' << '\n';
        PRINT_ARRAY(Vectors0);
        PRINT_ARRAY(Vectors2);

        Vectors0.Swap(Vectors2);

        std::cout << "After" << '\n' << '\n';
        PRINT_ARRAY(Vectors0);
        PRINT_ARRAY(Vectors2);

        // Fill 
        std::cout << '\n' << "Testing Fill" << '\n';
        Vectors0.Fill(FVec3(63.0f, 63.0f, 63.0f));
        PRINT_ARRAY(Vectors0);

        // Append
        std::cout << '\n' << "Testing Append" << '\n';
        Vectors0.Append(
            {
                FVec3(103.0f, 103.0f, 103.0f),
                FVec3(113.0f, 113.0f, 113.0f),
                FVec3(123.0f, 123.0f, 123.0f),
                FVec3(133.0f, 133.0f, 133.0f),
                FVec3(143.0f, 143.0f, 143.0f),
                FVec3(153.0f, 153.0f, 153.0f)
            });
        PRINT_ARRAY(Vectors0);

        // PopBackRange
        std::cout << '\n' << "Testing PopBackRange" << '\n';
        Vectors0.PopRange(3);
        PRINT_ARRAY(Vectors0);

        std::cout << '\n' << "Testing Equal operator" << '\n';
        TArray<FVec3, TArrayAllocator<FVec3>> EqualArray = Vectors0;
        std::cout << "operator==" << std::boolalpha << (EqualArray == Vectors0) << '\n';
    }
#endif

    // Test Heapify
    {
        std::cout << '\n' << "Testing Heapify" << '\n';

        {
            TArray<int32, TArrayAllocator<int32>> Heap = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };

            std::cout << '\n' << "Before" << '\n';
            PRINT_ARRAY(Heap);
            Heap.Heapify();

            std::cout << '\n' << "After" << '\n';
            PRINT_ARRAY(Heap);

            std::cout << '\n' << "Testing HeapPush" << '\n';
            Heap.HeapPush(19);
            PRINT_ARRAY(Heap);

            Heap.HeapPush(0);
            PRINT_ARRAY(Heap);

            std::cout << '\n' << "Testing HeapPop" << '\n';
            Heap.HeapPop();
            PRINT_ARRAY(Heap);
        }

        std::cout << '\n' << "Testing HeapSort" << '\n';
        {
            TArray<int32, TArrayAllocator<int32>> Heap = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };
            PRINT_ARRAY(Heap);
            Heap.HeapSort();
            PRINT_ARRAY(Heap);
        }
    }

    SUCCESS();
}

#endif // RUN_TARRAY_TEST

#endif // RUN_TARRAY_TEST || RUN_TARRAY_BENCHMARKS
