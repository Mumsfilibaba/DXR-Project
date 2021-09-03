#include "Array_Test.h"

#if RUN_TARRAY_TEST || RUN_TARRAY_BENCHMARKS
#include <Core/Containers/Array.h>

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

#define ENABLE_INLINE_ALLOCATOR (1)

#if ENABLE_INLINE_ALLOCATOR
template<typename T>
using TArrayAllocator = TInlineArrayAllocator<T, 128>;
#else
template<typename T>
using TArrayAllocator = TDefaultArrayAllocator<T>;
#endif

/*
* A clock
*/

struct SClock
{
    friend struct SScopedClock;

public:
    SClock()
        : Duration( 0 )
        , TotalDuration( 0 )
    {
    }

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
    inline void AddDuration( int64 InDuration )
    {
        Duration = InDuration;
        TotalDuration += Duration;
    }

    int64 Duration = 0;
    int64 TotalDuration = 0;
};

struct SScopedClock
{
    SScopedClock( SClock& InParent )
        : Parent( InParent )
        , t0( std::chrono::high_resolution_clock::now() )
        , t1()
    {
        t0 = std::chrono::high_resolution_clock::now();
    }

    ~SScopedClock()
    {
        t1 = std::chrono::high_resolution_clock::now();
        Parent.AddDuration( std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() );
    }

    SClock& Parent;
    std::chrono::steady_clock::time_point t0;
    std::chrono::steady_clock::time_point t1;
};

/*
 * Vec3
 */

struct Vec3
{
    Vec3() = default;

    Vec3( double InX, double InY, double InZ )
        : x( InX )
        , y( InY )
        , z( InZ )
    {
    }

    double x;
    double y;
    double z;

    operator std::string() const
    {
        return std::to_string( x ) + ", " + std::to_string( y ) + ", " + std::to_string( z );
    }

    bool operator==( const Vec3& Other ) const
    {
        return (x == Other.x) && (y == Other.y) && (z == Other.z);
    }
};

template<>
struct TIsReallocatable<std::string>
{
    enum
    {
        Value = true
    };
};

/*
 * PrintArr
 */

template<typename T>
void PrintArr( const TArray<T, TArrayAllocator<T>>& Arr, const std::string& Name = "" )
{
    std::cout << Name << std::endl;
    std::cout << "--------------------------------" << std::endl;

    for ( auto i : Arr )
    {
        std::cout << (std::string)i << std::endl;
    }

    std::cout << "Size: " << Arr.Size() << std::endl;
    std::cout << "Capacity: " << Arr.Capacity() << std::endl;

    std::cout << "--------------------------------" << std::endl << std::endl;
}

template<>
void PrintArr<int32>( const TArray<int32, TArrayAllocator<int32>>& Arr, const std::string& Name )
{
    std::cout << Name << std::endl;
    std::cout << "--------------------------------" << std::endl;

    for ( auto i : Arr )
    {
        std::cout << i << std::endl;
    }

    std::cout << "Size: " << Arr.Size() << std::endl;
    std::cout << "Capacity: " << Arr.Capacity() << std::endl;

    std::cout << "--------------------------------" << std::endl << std::endl;
}

template<typename T>
void PrintArr( const std::vector<T>& Arr, const std::string& Name = "" )
{
    std::cout << Name << std::endl;
    std::cout << "--------------------------------" << std::endl;

    for ( auto i : Arr )
    {
        std::cout << i << std::endl;
    }

    std::cout << "Size: " << Arr.size() << std::endl;
    std::cout << "Capacity: " << Arr.capacity() << std::endl;

    std::cout << "--------------------------------" << std::endl << std::endl;
}
#define PrintArr(Arr) PrintArr(Arr, #Arr)

/*
 * Benchmark
 */

void TArray_Benchmark()
{
    // Performance
    std::cout << std::endl << "Benchmark (std::string)" << std::endl;
#if defined(DEBUG_BUILD)
    const uint32 TestCount = 10;
#else
    const uint32 TestCount = 100;
#endif

    // Insert
#if 1
    {
	#if defined(DEBUG_BUILD) && defined(PLATFORM_WINDOWS)
        const uint32 Iterations = 1000;
    #else
        const uint32 Iterations = 10000;
    #endif
        std::cout << std::endl << "Insert/insert (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<std::string> Strings0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings0.insert( Strings0.begin(), "My name is jeff" );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                TArray<std::string, TArrayAllocator<std::string>> Strings1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings1.Insert( 0, "My name is jeff" );
                }
            }

            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

#if 1
    // EmplaceAt
    {
	#if defined(DEBUG_BUILD) && defined(PLATFORM_WINDOWS)
        const uint32 Iterations = 1000;
    #else
        const uint32 Iterations = 10000;
    #endif
        std::cout << std::endl << "EmplaceAt/emplace (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<std::string> Strings0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings0.emplace( Strings0.begin(), "My name is jeff" );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                TArray<std::string, TArrayAllocator<std::string>> Strings1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings1.EmplaceAt( 0, "My name is jeff" );
                }
            }
            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

#if 1
    // Push
    {
        const uint32 Iterations = 10000;
        std::cout << std::endl << "Push/push_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<std::string> Strings0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings0.push_back( "My name is jeff" );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
				TArray<std::string, TArrayAllocator<std::string>> Strings1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings1.Push( "My name is jeff" );
                }
            }
            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

#if 1
    // Emplace
    {
        const uint32 Iterations = 10000;
        std::cout << std::endl << "Emplace/emplace_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<std::string> Strings0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings0.emplace_back( "My name is jeff" );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
				TArray<std::string, TArrayAllocator<std::string>> Strings1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Strings1.Emplace( "My name is jeff" );
                }
            }
            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

    std::cout << std::endl << "Benchmark (Vec3)" << std::endl;

    // Insert
#if 1
    {
        const uint32 Iterations = 10000;
        std::cout << std::endl << "Insert/insert (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<Vec3> Vectors0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors0.insert( Vectors0.begin(), Vec3( 3.0, 5.0, -6.0 ) );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
				TArray<Vec3, TArrayAllocator<Vec3>> Vectors1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors1.Insert( 0, Vec3( 3.0, 5.0, -6.0 ) );
                }
            }
            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

#if 1
    // EmplaceAt
    {
        const uint32 Iterations = 10000;
        std::cout << std::endl << "EmplaceAt/emplace (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<Vec3> Vectors0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors0.emplace( Vectors0.begin(), 3.0, 5.0, -6.0 );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
				TArray<Vec3, TArrayAllocator<Vec3>> Vectors1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors1.EmplaceAt( 0, double( j + 1 ), 5.0, -6.0 );
                }
            }
            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

#if 1
    // Push
    {
        const uint32 Iterations = 10000;
        std::cout << std::endl << "Push/push_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<Vec3> Vectors0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors0.push_back( Vec3( 3.0, 5.0, -6.0 ) );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
				TArray<Vec3, TArrayAllocator<Vec3>> Vectors1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors1.Push( Vec3( 3.0, 5.0, -6.0 ) );
                }
            }
            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

#if 1
    // Emplace
    {
        const uint32 Iterations = 10000;
        std::cout << std::endl << "Emplace/emplace_back (Iterations=" << Iterations << ", TestCount=" << TestCount << ")" << std::endl;
        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
                std::vector<Vec3> Vectors0;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors0.emplace_back( 3.0, 5.0, -6.0 );
                }
            }

            std::cout << "std::vector:" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }

        {
            SClock Clock;
            for ( uint32 i = 0; i < TestCount; i++ )
            {
				TArray<Vec3, TArrayAllocator<Vec3>> Vectors1;

                SScopedClock SScopedClock( Clock );
                for ( uint32 j = 0; j < Iterations; j++ )
                {
                    Vectors1.Emplace( 3.0, 5.0, -6.0 );
                }
            }

            std::cout << "TArray     :" << Clock.GetTotalDuration() / TestCount << "ns" << std::endl;
        }
    }
#endif

#if 1
    {
#if defined(DEBUG_BUILD)
		const uint32 SortTestCount = 10;
#else
		const uint32 SortTestCount = 100;
#endif
		
        const uint32 NumNumbers = 1000000;
        std::cout << std::endl << "HeapSort/heap_sort (NumNumbers=" << NumNumbers << ", SortTestCount=" << SortTestCount << ")" << std::endl;

        srand( (unsigned int)time( 0 ) );

		{
			SClock Clock;
			for ( uint32 i = 0; i < SortTestCount; i++ )
			{
				TArray<int32, TArrayAllocator<int32>> Heap;
				for ( uint32 n = 0; n < NumNumbers; n++ )
				{
					Heap.Emplace( rand() );
				}

				{
					SScopedClock SScopedClock( Clock );
					Heap.HeapSort();
				}
			}

			auto Duration = Clock.GetTotalDuration() / SortTestCount;
			std::cout << "TArray      Sorting time: " << Duration << "ns" << std::endl;
			std::cout << "TArray      Sorting time: " << Duration / (1000 * 1000) << "ms" << std::endl;
		}
		
		{
			SClock Clock;
			for ( uint32 i = 0; i < SortTestCount; i++ )
			{
				std::vector<int32> Heap;
				for ( uint32 n = 0; n < NumNumbers; n++ )
				{
					Heap.emplace_back( rand() );
				}

				{
					SScopedClock SScopedClock( Clock );
					std::make_heap(Heap.begin(), Heap.end());
					std::sort_heap(Heap.begin(), Heap.end());
				}
			}

			auto Duration = Clock.GetTotalDuration() / SortTestCount;
			std::cout << "std::vector Sorting time: " << Duration << "ns" << std::endl;
			std::cout << "std::vector Sorting time: " << Duration / (1000 * 1000) << "ms" << std::endl;
		}
    }
#endif
}

/*
 * Test
 */

void TArray_Test( int32 Argc, const char** Argv )
{
#if 1
    std::cout << std::endl << "----------TArray----------" << std::endl << std::endl;
    {
        std::string ArgvStr = Argc > 0 ? Argv[0] : "Filepath";

        std::cout << "Testing TArray<std::string>" << std::endl;

        std::cout << std::endl << "Testing Constructors" << std::endl << std::endl;
        // Test constructors
        TArray<std::string, TArrayAllocator<std::string>> Strings0;
        TArray<std::string, TArrayAllocator<std::string>> Strings1( 5, "Hello" );
        TArray<std::string, TArrayAllocator<std::string>> Strings2( Strings1.Data(), Strings1.Size() );
        TArray<std::string, TArrayAllocator<std::string>> Strings3 =
        {
            "Hello World",
            "TArray",
            "This is a longer teststring"
        };

        PrintArr( Strings0 );
        PrintArr( Strings1 );
        PrintArr( Strings2 );
        PrintArr( Strings3 );

        {
            std::cout << "Test Copy Constructor" << std::endl << std::endl;

            // Test copy an empty array
            TArray<std::string, TArrayAllocator<std::string>> Strings4 = Strings0;
            // Test copy an array with data
            TArray<std::string, TArrayAllocator<std::string>> Strings5 = Strings1;

            PrintArr( Strings4 );

            std::cout << "Before move" << std::endl << std::endl;
            PrintArr( Strings5 );

            // Test move an array with data
            std::cout << "Test Move Constructor" << std::endl << std::endl;

            TArray<std::string, TArrayAllocator<std::string>> Strings6 = Move( Strings5 );

            std::cout << "After move" << std::endl << std::endl;
            PrintArr( Strings5 );
            PrintArr( Strings6 );
        }

        // Reset
        std::cout << std::endl << "Testing Reset" << std::endl << std::endl;
        Strings0.Reset( 7, "This is a teststring" );
        PrintArr( Strings0 );

        Strings1.Reset( { "Test-String #1", "Test-String #2", "Test-String #3" } );
        PrintArr( Strings1 );

        Strings2.Reset( Strings3.Data(), Strings3.Size() );
        PrintArr( Strings2 );

        // Resize
        std::cout << std::endl << "Testing Resize" << std::endl << std::endl;

        // Constructing a empty Array to test resize
        TArray<std::string, TArrayAllocator<std::string>> Strings4;

        std::cout << "Before Resize" << std::endl << std::endl;
        PrintArr( Strings4 );
        PrintArr( Strings3 );
        PrintArr( Strings1 );

        Strings4.Resize( 10, "New String" );
        Strings3.Resize( 4, "Hi, hi" );
        Strings1.Resize( 6, "Hello World" );

        std::cout << "After Resize" << std::endl << std::endl;
        PrintArr( Strings4 );
        PrintArr( Strings3 );
        PrintArr( Strings1 );

        std::cout << "Testing Shrinking Resize" << std::endl << std::endl;
        Strings4.Resize( 2, "New String" );
        PrintArr( Strings4 );

        std::cout << "Testing Growing Resize" << std::endl << std::endl;
        Strings4.Resize( 15, "New String" );
        PrintArr( Strings4 );

        // Reserve
        std::cout << std::endl << "Testing Reserve" << std::endl << std::endl;

        std::cout << "Before Reserve" << std::endl << std::endl;
        PrintArr( Strings4 );

        std::cout << "After Reserve" << std::endl << std::endl;
        Strings4.Reserve( Strings4.Capacity() );
        PrintArr( Strings4 );

        std::cout << "Shrinking" << std::endl << std::endl;
        Strings4.Reserve( 5 );
        PrintArr( Strings4 );

        std::cout << "Growing" << std::endl << std::endl;
        Strings4.Reserve( 10 );
        PrintArr( Strings4 );

        std::cout << "Resize" << std::endl << std::endl;
        Strings4.Resize( Strings4.Capacity() - 2, "This spot is reserved" );
        PrintArr( Strings4 );

        // Shrink To Fit
        std::cout << std::endl << "Testing ShrinkToFit" << std::endl << std::endl;

        std::cout << std::endl << "Before ShrinkToFit" << std::endl << std::endl;
        PrintArr( Strings4 );

        Strings4.ShrinkToFit();

        std::cout << std::endl << "After ShrinkToFit" << std::endl << std::endl;
        PrintArr( Strings4 );

        // Assignment
        std::cout << std::endl << "Testing Assignment" << std::endl << std::endl;

        Strings3.Resize( 5, "No i am your father" );

        std::cout << "Before Assignment" << std::endl << std::endl;
        PrintArr( Strings0 );
        PrintArr( Strings1 );
        PrintArr( Strings2 );
        PrintArr( Strings3 );
        PrintArr( Strings4 );

        std::cout << "Strings0 = Strings4" << std::endl;
        Strings0 = Strings4;

        std::cout << "Strings1 = Move(Strings3)" << std::endl;
        Strings1 = Move( Strings3 );

        std::cout << "Strings2 = InitializerList" << std::endl << std::endl;
        Strings2 =
        {
            "Another String in a InitializerList",
            "Strings are kinda cool",
            "Letters to fill up space in a string"
        };

        std::cout << "After Assignment" << std::endl << std::endl;
        PrintArr( Strings0 );
        PrintArr( Strings1 );
        PrintArr( Strings2 );
        PrintArr( Strings3 );
        PrintArr( Strings4 );

        // PushBack
        std::cout << std::endl << "Testing PushBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 6; i++ )
        {
            Strings2.Push( "This is Pushed String #" + std::to_string( i ) );
        }
        PrintArr( Strings2 );

        std::cout << std::endl << "Testing PushBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 6; i++ )
        {
            Strings2.Push( ArgvStr );
        }
        PrintArr( Strings2 );

        // EmplaceBack
        std::cout << std::endl << "Testing EmplaceBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 6; i++ )
        {
            Strings2.Emplace( "This is an Emplaced String #" + std::to_string( i ) );
        }
        PrintArr( Strings2 );

        // PopBack
        std::cout << std::endl << "Testing PopBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 3; i++ )
        {
            Strings2.Pop();
        }
        PrintArr( Strings2 );


        // Insert
        std::cout << std::endl << "Testing Insert" << std::endl << std::endl;

        std::cout << "Before Insert" << std::endl << std::endl;
        PrintArr( Strings2 );

        std::cout << "At front" << std::endl << std::endl;
        Strings2.Insert( 0, ArgvStr );
        PrintArr( Strings2 );
        Strings2.Insert( 0, "Inserted String" );
        PrintArr( Strings2 );
        Strings2.Insert( 0, { "Inserted String #1", "Inserted String #2" } );
        PrintArr( Strings2 );

        std::cout << "At Arbitrary" << std::endl << std::endl;
        Strings2.Insert( 2, ArgvStr );
        PrintArr( Strings2 );
        Strings2.Insert( 2, "Inserted String Again" );
        PrintArr( Strings2 );
        Strings2.Insert( 2, { "Inserted String Again #1", "Inserted String Again #2" } );
        PrintArr( Strings2 );

        std::cout << "At End" << std::endl << std::endl;
        Strings2.Insert( Strings2.Size(), { "Inserted String At End #1", "Inserted String At End #2" } );
        PrintArr( Strings2 );

        std::cout << "At front after reallocation" << std::endl << std::endl;
        // Add a shrink to fit to force reallocation
        Strings2.ShrinkToFit();
        Strings2.Insert( 0, ArgvStr );
        PrintArr( Strings2 );
        // Add a shrink to fit to force reallocation
        Strings2.ShrinkToFit();
        Strings2.Insert( 0, "Inserted String Reallocated" );
        PrintArr( Strings2 );
        // Add a shrink to fit to force reallocation
        Strings2.ShrinkToFit();
        Strings2.Insert( 0, { "Inserted String Reallocated #1", "Inserted String Reallocated #2" } );
        PrintArr( Strings2 );

        std::cout << "At Arbitrary after reallocation" << std::endl << std::endl;
        // Add a shrink to fit to force reallocation
        Strings2.ShrinkToFit();
        Strings2.Insert( 2, ArgvStr );
        PrintArr( Strings2 );
        // Add a shrink to fit to force reallocation
        Strings2.ShrinkToFit();
        Strings2.Insert( 2, "Inserted String Again Reallocated" );
        PrintArr( Strings2 );
        // Add a shrink to fit to force reallocation
        Strings2.ShrinkToFit();
        Strings2.Insert( 2, { "Inserted String Again Reallocated #1", "Inserted String Again Reallocated #2" } );
        PrintArr( Strings2 );

        std::cout << "At End after reallocation" << std::endl << std::endl;
        // Add a shrink to fit to force reallocation
        Strings2.ShrinkToFit();
        Strings2.Insert( Strings2.Size(), { "Inserted String At End Reallocated #1", "Inserted String At End Reallocated #2" } );
        PrintArr( Strings2 );

        // Erase
        std::cout << std::endl << "Testing Erase" << std::endl << std::endl;
        std::cout << "Before Erase" << std::endl << std::endl;
        PrintArr( Strings2 );

        std::cout << "At front" << std::endl << std::endl;
        Strings2.RemoveAt( 0 );
        PrintArr( Strings2 );

        std::cout << "At Arbitrary" << std::endl << std::endl;
        Strings2.RemoveAt( 2 );
        PrintArr( Strings2 );

        std::cout << "Range At front" << std::endl << std::endl;
        Strings2.RemoveRangeAt( 0, 2 );
        PrintArr( Strings2 );

        std::cout << "Range At Arbitrary" << std::endl << std::endl;
        Strings2.RemoveRangeAt( 4, 3 );
        PrintArr( Strings2 );

        std::cout << "Range At End" << std::endl << std::endl;
        Strings2.RemoveRangeAt( Strings2.Size() - 3, 3 );
        PrintArr( Strings2 );

        std::cout << "Testing Erase In Loop" << std::endl << std::endl;
        TArray<std::string, TArrayAllocator<std::string>> LoopStrings =
        {
            "Str0",
            "Str1",
            "Str2",
            "Str3",
            "Str4",
            "Str5",
        };

        std::cout << "Before" << std::endl << std::endl;
        PrintArr( LoopStrings );

        uint32 Index = 0;
        for ( TArray<std::string, TArrayAllocator<std::string>>::IteratorType It = LoopStrings.StartIterator(); It != LoopStrings.EndIterator();)
        {
            if ( Index > 1 )
            {
                It = LoopStrings.RemoveAt( It );
            }
            else
            {
                It++;
            }

            Index++;
        }

        std::cout << "After" << std::endl << std::endl;
        PrintArr( LoopStrings );

        // Swap
        std::cout << std::endl << "Testing Swap" << std::endl << std::endl;
        std::cout << "Before" << std::endl << std::endl;
        PrintArr( Strings0 );
        PrintArr( Strings2 );

        Strings0.Swap( Strings2 );

        std::cout << "After" << std::endl << std::endl;
        PrintArr( Strings0 );
        PrintArr( Strings2 );

        // Iterators
        std::cout << std::endl << "Testing Iterators" << std::endl;

        std::cout << std::endl << "Iterators" << std::endl << std::endl;
        for ( auto It = Strings2.StartIterator(); It != Strings2.EndIterator(); It++ )
        {
            std::cout << (*It) << std::endl;
        }

        TArray<std::string, TArrayAllocator<std::string>> EmptyArray;
        for ( auto It = EmptyArray.StartIterator(); It != EmptyArray.EndIterator(); It++ )
        {
            std::cout << (*It) << std::endl;
        }

        std::cout << std::endl;

        for ( std::string& Str : Strings2 )
        {
            std::cout << Str << std::endl;
        }

        std::cout << std::endl << "Reverse Iterators" << std::endl << std::endl;
        for ( auto It = Strings2.ReverseStartIterator(); It != Strings2.ReverseEndIterator(); It++ )
        {
            std::cout << (*It) << std::endl;
        }

        // Fill 
        std::cout << std::endl << "Testing Fill" << std::endl;
        Strings2.Fill( "Fill Value" );
        PrintArr( Strings2 );

        // Append
        std::cout << std::endl << "Testing Append" << std::endl;
        Strings2.Append( { "Append #1", "Append #2", "Append #3", "Append #4", "Append #5", "Append #6" } );
        PrintArr( Strings2 );

        // PopBackRange
        std::cout << std::endl << "Testing PopBackRange" << std::endl;
        Strings2.PopRange( 3 );
        PrintArr( Strings2 );

        std::cout << std::endl << "Testing Equal operator" << std::endl;
        TArray<std::string, TArrayAllocator<std::string>> EqualArray = Strings2;
        std::cout << "operator==" << std::boolalpha << (EqualArray == Strings2) << std::endl;
    }
#endif

    // TArray<vec3>
#if 1
    {
        std::cout << std::endl << "Testing TArray<Vec3, TArrayAllocator<Vec3>>" << std::endl;
        std::cout << std::endl << "Testing Constructors" << std::endl;
        // Test constructors
        TArray<Vec3, TArrayAllocator<Vec3>> Vectors0;
        TArray<Vec3, TArrayAllocator<Vec3>> Vectors1( 5, Vec3( 1.0, 1.0, 1.0 ) );
        TArray<Vec3, TArrayAllocator<Vec3>> Vectors2( Vectors1.Data(), Vectors1.Size() );
        TArray<Vec3, TArrayAllocator<Vec3>> Vectors3 =
        {
            Vec3( 1.0, 1.0, 1.0 ),
            Vec3( 2.0, 2.0, 2.0 ),
            Vec3( 3.0, 3.0, 3.0 )
        };

        PrintArr( Vectors0 );
        PrintArr( Vectors1 );
        PrintArr( Vectors2 );
        PrintArr( Vectors3 );

        {
            // Test copy an empty array
            TArray<Vec3, TArrayAllocator<Vec3>> Vectors4 = Vectors0;
            // Test copy an array with data
            TArray<Vec3, TArrayAllocator<Vec3>> Vectors5 = Vectors1;

            PrintArr( Vectors4 );

            std::cout << "Before move" << std::endl << std::endl;
            PrintArr( Vectors5 );

            // Test move an array with data
            TArray<Vec3, TArrayAllocator<Vec3>> Vectors6 = Move( Vectors5 );

            std::cout << "After move" << std::endl << std::endl;
            PrintArr( Vectors5 );
            PrintArr( Vectors6 );
        }

        // Assign
        std::cout << std::endl << "Testing Assign" << std::endl;
        Vectors0.Reset( 7, Vec3( 5.0, 5.0, 5.0 ) );
        PrintArr( Vectors0 );

        Vectors1.Reset( { Vec3( 1.0, 5.0, 5.0 ), Vec3( 2.0, 5.0, 5.0 ), Vec3( 3.0, 5.0, 5.0 ) } );
        PrintArr( Vectors1 );

        Vectors2.Reset( Vectors3.Data(), Vectors3.Size() );
        PrintArr( Vectors2 );

        // Resize
        std::cout << std::endl << "Testing Resize" << std::endl << std::endl;

        // Constructing a empty Array to test resize
        TArray<Vec3, TArrayAllocator<Vec3>> Vectors4;

        std::cout << "Before Resize" << std::endl << std::endl;
        PrintArr( Vectors4 );
        PrintArr( Vectors3 );
        PrintArr( Vectors1 );

        Vectors4.Resize( 10, Vec3( -10.0, -10.0, -10.0 ) );
        Vectors3.Resize( 0 );
        Vectors1.Resize( 6, Vec3( -5.0, 10.0, -15.0 ) );

        std::cout << "After Resize" << std::endl << std::endl;
        PrintArr( Vectors4 );
        PrintArr( Vectors3 );
        PrintArr( Vectors1 );

        std::cout << "Testing Shrinking Resize" << std::endl << std::endl;
        Vectors4.Resize( 2, Vec3( -15.0, -15.0, -15.0 ) );
        PrintArr( Vectors4 );

        Vectors4.Resize( 15, Vec3( 23.0, 23.0, 23.0 ) );
        PrintArr( Vectors4 );

        // Reserve
        std::cout << std::endl << "Testing Reserve" << std::endl << std::endl;

        std::cout << "Before Reserve" << std::endl << std::endl;
        PrintArr( Vectors4 );

        std::cout << "After Reserve" << std::endl << std::endl;
        Vectors4.Reserve( Vectors4.Capacity() );
        PrintArr( Vectors4 );

        std::cout << "Shrinking" << std::endl << std::endl;
        Vectors4.Reserve( 5 );
        PrintArr( Vectors4 );

        std::cout << "Growing" << std::endl << std::endl;
        Vectors4.Reserve( 10 );
        PrintArr( Vectors4 );

        std::cout << "Resize" << std::endl << std::endl;
        Vectors4.Resize( Vectors4.Capacity() - 2, Vec3( -1.0f, -1.0f, -1.0f ) );
        PrintArr( Vectors4 );

        // Shrink To Fit
        std::cout << std::endl << "Testing ShrinkToFit" << std::endl;

        std::cout << std::endl << "Before ShrinkToFit" << std::endl << std::endl;
        PrintArr( Vectors4 );

        Vectors4.ShrinkToFit();

        std::cout << std::endl << "After ShrinkToFit" << std::endl << std::endl;
        PrintArr( Vectors4 );

        // Assignment
        std::cout << std::endl << "Testing Assignment" << std::endl << std::endl;

        Vectors3.Resize( 3, Vec3( 42.0, 42.0, 42.0 ) );

        std::cout << "Before Assignment" << std::endl << std::endl;
        PrintArr( Vectors0 );
        PrintArr( Vectors1 );
        PrintArr( Vectors2 );
        PrintArr( Vectors3 );
        PrintArr( Vectors4 );

        std::cout << "Vectors0 = Vectors4" << std::endl;
        Vectors0 = Vectors4;

        std::cout << "Vectors1 = Move(Vectors3)" << std::endl;
        Vectors1 = Move( Vectors3 );

        std::cout << "Vectors2 = InitializerList" << std::endl << std::endl;
        Vectors2 =
        {
            Vec3( 9.0, 9.0, 9.0 ),
            Vec3( 10.0, 10.0, 10.0 ),
            Vec3( 11.0, 11.0, 11.0 )
        };

        std::cout << "After Assignment" << std::endl << std::endl;
        PrintArr( Vectors0 );
        PrintArr( Vectors1 );
        PrintArr( Vectors2 );
        PrintArr( Vectors3 );
        PrintArr( Vectors4 );

        // PushBack
        std::cout << std::endl << "Testing PushBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 6; i++ )
        {
            Vectors2.Push( Vec3( 7.0, 7.0, 7.0 ) );
        }
        PrintArr( Vectors2 );

        Vec3 Vector( 5.0f, -45.0f, 5.0f );
        std::cout << std::endl << "Testing PushBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 6; i++ )
        {
            Vectors2.Push( Vector );
        }
        PrintArr( Vectors2 );

        // EmplaceBack
        std::cout << std::endl << "Testing EmplaceBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 6; i++ )
        {
            Vectors2.Emplace( 1.0, 0.0, 1.0 );
        }
        PrintArr( Vectors2 );

        // PopBack
        std::cout << std::endl << "Testing PopBack" << std::endl << std::endl;
        for ( uint32 i = 0; i < 3; i++ )
        {
            Vectors2.Pop();
        }
        PrintArr( Vectors2 );

        // Insert
        std::cout << std::endl << "Testing Insert" << std::endl << std::endl;
        std::cout << "At front" << std::endl << std::endl;
        Vectors2.Insert( 0, Vector );
        Vectors2.Insert( 0, Vec3( -1.0, -1.0, -1.0 ) );
        Vectors2.Insert( 0, { Vec3( 1.0f, 1.0f, 1.0f ), Vec3( 2.0f, 2.0f, 2.0f ) } );
        PrintArr( Vectors2 );

        std::cout << "At Arbitrary" << std::endl << std::endl;
        Vectors2.Insert( 2, Vector );
        Vectors2.Insert( 2, Vec3( -1.0, -1.0, -2.0 ) );
        Vectors2.Insert( 2, { Vec3( 1.0f, 1.0f, 2.0f ), Vec3( 2.0f, 2.0f, 3.0f ) } );
        PrintArr( Vectors2 );

        std::cout << "At End" << std::endl << std::endl;
        Vectors2.Insert( Vectors2.Size(), { Vec3( 1.0f, 1.0f, 3.0f ), Vec3( 2.0f, 2.0f, 4.0f ) } );
        PrintArr( Vectors2 );

        std::cout << "At front after reallocation" << std::endl << std::endl;
        // Add a shrink to fit to force reallocation
        Vectors2.ShrinkToFit();
        Vectors2.Insert( 0, Vector );
        // Add a shrink to fit to force reallocation
        Vectors2.ShrinkToFit();
        Vectors2.Insert( 0, Vec3( -1.0, -1.0, -3.0 ) );
        // Add a shrink to fit to force reallocation
        Vectors2.ShrinkToFit();
        Vectors2.Insert( 0, { Vec3( 1.0f, 1.0f, 4.0f ), Vec3( 2.0f, 2.0f, 5.0f ) } );
        PrintArr( Vectors2 );

        std::cout << "At Arbitrary after reallocation" << std::endl << std::endl;
        // Add a shrink to fit to force reallocation
        Vectors2.ShrinkToFit();
        Vectors2.Insert( 2, Vector );
        // Add a shrink to fit to force reallocation
        Vectors2.ShrinkToFit();
        Vectors2.Insert( 2, Vec3( -1.0, -1.0, -4.0 ) );
        // Add a shrink to fit to force reallocation
        Vectors2.ShrinkToFit();
        Vectors2.Insert( 2, { Vec3( 1.0f, 1.0f, 5.0f ), Vec3( 2.0f, 2.0f, 6.0f ) } );
        PrintArr( Vectors2 );

        std::cout << "At End after reallocation" << std::endl << std::endl;
        // Add a shrink to fit to force reallocation
        Vectors2.ShrinkToFit();
        Vectors2.Insert( Vectors2.Size(), { Vec3( 6.0f, 6.0f, 6.0f ), Vec3( 2.0f, 2.0f, 7.0f ) } );
        PrintArr( Vectors2 );

        // Erase
        std::cout << std::endl << "Testing Erase" << std::endl << std::endl;
        PrintArr( Vectors2 );

        std::cout << "At front" << std::endl << std::endl;
        Vectors2.RemoveAt( 0 );
        PrintArr( Vectors2 );

        std::cout << "At Arbitrary" << std::endl << std::endl;
        Vectors2.RemoveAt( 2 );
        PrintArr( Vectors2 );

        std::cout << "Range At front" << std::endl << std::endl;
        Vectors2.RemoveRangeAt( 0, 2 );
        PrintArr( Vectors2 );

        std::cout << "Range At Arbitrary" << std::endl << std::endl;
        Vectors2.RemoveRangeAt( 4, 3 );
        PrintArr( Vectors2 );

        std::cout << "Range At End" << std::endl << std::endl;
        Vectors2.RemoveRangeAt( Vectors2.Size() - 3, 3 );
        PrintArr( Vectors2 );

        // Swap
        std::cout << std::endl << "Testing Swap" << std::endl << std::endl;
        std::cout << "Before" << std::endl << std::endl;
        PrintArr( Vectors0 );
        PrintArr( Vectors2 );

        Vectors0.Swap( Vectors2 );

        std::cout << "After" << std::endl << std::endl;
        PrintArr( Vectors0 );
        PrintArr( Vectors2 );

        // Fill 
        std::cout << std::endl << "Testing Fill" << std::endl;
        Vectors0.Fill( Vec3( 63.0f, 63.0f, 63.0f ) );
        PrintArr( Vectors0 );

        // Append
        std::cout << std::endl << "Testing Append" << std::endl;
        Vectors0.Append(
            {
                Vec3( 103.0f, 103.0f, 103.0f ),
                Vec3( 113.0f, 113.0f, 113.0f ),
                Vec3( 123.0f, 123.0f, 123.0f ),
                Vec3( 133.0f, 133.0f, 133.0f ),
                Vec3( 143.0f, 143.0f, 143.0f ),
                Vec3( 153.0f, 153.0f, 153.0f )
            } );
        PrintArr( Vectors0 );

        // PopBackRange
        std::cout << std::endl << "Testing PopBackRange" << std::endl;
        Vectors0.PopRange( 3 );
        PrintArr( Vectors0 );

        std::cout << std::endl << "Testing Equal operator" << std::endl;
        TArray<Vec3, TArrayAllocator<Vec3>> EqualArray = Vectors0;
        std::cout << "operator==" << std::boolalpha << (EqualArray == Vectors0) << std::endl;
    }
#endif

    // Test Heapify
    {
        std::cout << std::endl << "Testing Heapify" << std::endl;

        {
            TArray<int32, TArrayAllocator<int32>> Heap = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };

            std::cout << std::endl << "Before" << std::endl;
            PrintArr( Heap );
            Heap.Heapify();

            std::cout << std::endl << "After" << std::endl;
            PrintArr( Heap );

            std::cout << std::endl << "Testing HeapPush" << std::endl;
            Heap.HeapPush( 19 );
            PrintArr( Heap );

            Heap.HeapPush( 0 );
            PrintArr( Heap );

            std::cout << std::endl << "Testing HeapPop" << std::endl;
            Heap.HeapPop();
            PrintArr( Heap );
        }

        std::cout << std::endl << "Testing HeapSort" << std::endl;
        {
			TArray<int32, TArrayAllocator<int32>> Heap = { 1, 3, 5, 4, 6, 13, 10, 9, 8, 15, 17 };
            PrintArr( Heap );
            Heap.HeapSort();
            PrintArr( Heap );
        }
    }
}

#endif
