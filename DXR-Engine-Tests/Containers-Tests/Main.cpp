#include <CoreTypes.h>

#include "TArray_Test.h"
#include "TSharedPtr_Test.h"
#include "TFunction_Test.h"
#include "TFixedArray_Test.h"
#include "TArrayView_Test.h"

/* Defines */
#define RUN_TESTS     1
#define RUN_BENCHMARK 1

/* Test Specific defines */
#define RUN_TARRAY_TEST      1
#define RUN_TSHAREDPTR_TEST  1
#define RUN_TFUNCTION_TEST   1
#define RUN_TFIXEDARRAY_TEST 1
#define RUN_TARRAYVIEW_TEST  1

/* Benchmark Specific defines */
#define RUN_TARRAY_BENCHMARKS 1

/* Check for memory leaks */
#ifdef PLATFORM_WINDOWS
#include <crtdbg.h>
#endif

/*
* Benchmarks
*/

void BenchMarks()
{
#if RUN_TARRAY_BENCHMARKS
    TArray_Benchmark();
#endif
}

/*
* Tests
*/

void Tests( int32 Argc, const char* Argv[] )
{
#if RUN_TARRAY_TEST
    TArray_Test( Argc, Argv );
#endif

#if RUN_TSHAREDPTR_TEST
    TSharedPtr_Test();
#endif

#if RUN_TFUNCTION_TEST
    TFunction_Test();
#endif

#if RUN_TFIXEDARRAY_TEST
    TStaticArray_Test();
#endif

#if RUN_TARRAYVIEW_TEST
    TArrayView_Test();
#endif
}

/*
* Main
*/

int main( int Argc, const char* Argv[] )
{
#ifdef _WIN32
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

#if RUN_TESTS
    Tests( Argc, Argv );
#endif

#if RUN_BENCHMARK
    BenchMarks();
#endif

    return 0;
}
