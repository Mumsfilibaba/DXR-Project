#include <CoreTypes.h>
#include <CoreDefines.h>

#include "Array_Test.h"
#include "SharedPtr_Test.h"
#include "Function_Test.h"
#include "FixedArray_Test.h"
#include "ArrayView_Test.h"
#include "Delegate_Test.h"
#include "String_Test.h"

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
    UNREFERENCED_VARIABLE( Argc );
    UNREFERENCED_VARIABLE( Argv );

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
    TFixedArray_Test();
#endif

#if RUN_TARRAYVIEW_TEST
    TArrayView_Test();
#endif

#if RUN_DELEGATE_TEST
    Delegate_Test();
#endif

#if RUN_TSTRING_TEST
    TString_Test( Argv[0] );
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