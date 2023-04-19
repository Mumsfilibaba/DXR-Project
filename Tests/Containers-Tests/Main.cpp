#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Platform/PlatformMisc.h>

#include "Array_Test.h"
#include "SharedPtr_Test.h"
#include "Function_Test.h"
#include "StaticArray_Test.h"
#include "ArrayView_Test.h"
#include "Delegate_Test.h"
#include "String_Test.h"
#include "Optional_Test.h"
#include "Queue_Test.h"
#include "Variant_Test.h"
#include "BitArray_Test.h"

/**
 *  Check for memory leaks 
 */
#ifdef PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

/**
 * Benchmarks
 */

void BenchMarks()
{
#if RUN_TARRAY_BENCHMARKS
    TArray_Benchmark();
#endif
}

/**
 * Tests
 */

DISABLE_UNREFERENCED_VARIABLE_WARNING

void Tests(int32 Argc, const CHAR* Argv[])
{
#if RUN_TARRAY_TEST
    TArray_Test(Argc, Argv);
#endif

#if RUN_TSHAREDPTR_TEST
    TSharedPtr_Test();
#endif

#if RUN_TFUNCTION_TEST
    TFunction_Test();
#endif

#if RUN_TSTATICARRAY_TEST
    TStaticArray_Test();
#endif

#if RUN_TARRAYVIEW_TEST
    TArrayView_Test();
#endif

#if RUN_TDELEGATE_TEST
    TDelegate_Test();
#endif

#if (RUN_TSTRING_TEST || RUN_TSTATICSTRING_TEST || RUN_TSTRINGVIEW_TEST)
    TString_Test(Argv[0]);
#endif

#if RUN_TOPIONAL_TEST
    TOptional_Test();
#endif

#if RUN_TQUEUE_TEST
    TQueue_Test();
#endif

#if RUN_TVARIANT_TEST
    TVariant_Test();
#endif

#if RUN_TBITARRAY_TEST
    TBitArray_Test();
#endif

#if RUN_TSTATICBITARRAY_TEST
    TStaticBitArray_Test();
#endif
}

/**
 * Main
 */
struct FDebuggerOutputDevice : public IOutputDevice
{
    virtual void Log(const FString& Message)
    {
        FPlatformMisc::OutputDebugString(Message.GetCString());
        FPlatformMisc::OutputDebugString("\n");
    }

    virtual void Log(ELogSeverity Severity, const FString& Message)
    {
        Log(Message);
    }
};

int main(int Argc, const CHAR* Argv[])
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#if RUN_TESTS
    Tests(Argc, Argv);
#endif

#if RUN_BENCHMARK
    BenchMarks();
#endif

    // NOTE: If this reports no memory-leaks, that is probably false positives
    if (GMalloc)
    {
        FDebuggerOutputDevice OutputDevice;
        GMalloc->DumpAllocations(&OutputDevice);
    }

    FPlatformStackTrace::ReleaseSymbols();
    return 0;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING