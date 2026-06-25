#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Platform/PlatformMisc.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

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
#include "Map_Test.h"
#include "Set_Test.h"
#include "UniquePtr_Test.h"
#include "PriorityQueue_Test.h"
#include "LinkedList_Test.h"
#include "StringView_Test.h"
#include "StaticString_Test.h"
#include "CRC_Test.h"

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
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

#if RUN_TARRAY_TEST
    RUN_TEST("Array", TArray_Test());
#endif

#if RUN_TSHAREDPTR_TEST
    RUN_TEST("SharedPtr", TSharedPtr_Test());
#endif

#if RUN_TFUNCTION_TEST
    RUN_TEST("Function", TFunction_Test());
#endif

#if RUN_TSTATICARRAY_TEST
    RUN_TEST("StaticArray", TStaticArray_Test());
#endif

#if RUN_TARRAYVIEW_TEST
    RUN_TEST("ArrayView", TArrayView_Test());
#endif

#if RUN_TDELEGATE_TEST
    RUN_TEST("Delegate", TDelegate_Test());
#endif

#if RUN_TSTRING_TEST
    RUN_TEST("String", TString_Test());
#endif

#if RUN_TOPIONAL_TEST
    RUN_TEST("Optional", TOptional_Test());
#endif

#if RUN_TQUEUE_TEST
    RUN_TEST("Queue", TQueue_Test());
#endif

#if RUN_TVARIANT_TEST
    RUN_TEST("Variant", TVariant_Test());
#endif

#if RUN_TBITARRAY_TEST
    RUN_TEST("BitArray", TBitArray_Test());
#endif

#if RUN_TSTATICBITARRAY_TEST
    RUN_TEST("StaticBitArray", TStaticBitArray_Test());
#endif

#if RUN_TMAP_TEST
    RUN_TEST("Map", TMap_Test());
#endif

#if RUN_TSET_TEST
    RUN_TEST("Set", TSet_Test());
#endif

#if RUN_TUNIQUEPTR_TEST
    RUN_TEST("UniquePtr", TUniquePtr_Test());
#endif

#if RUN_TPRIORITYQUEUE_TEST
    RUN_TEST("PriorityQueue", TPriorityQueue_Test());
#endif

#if RUN_TLINKEDLIST_TEST
    RUN_TEST("LinkedList", TLinkedList_Test());
#endif

#if RUN_STRINGVIEW_SUITE
    RUN_TEST("StringView", StringView_Suite());
#endif

#if RUN_STATICSTRING_SUITE
    RUN_TEST("StaticString", StaticString_Suite());
#endif

#if RUN_CRC_TEST
    RUN_TEST("CRC32", CRC_Test());
#endif
}

/**
 * Main
 */
struct FDebuggerOutputDevice : public IOutputDevice
{
    virtual void Log(const String& Message)
    {
        FPlatformMisc::OutputDebugString(Message.Data());
        FPlatformMisc::OutputDebugString("\n");
    }

    virtual void Log(ELogSeverity Severity, const String& Message)
    {
        Log(Message);
    }
};

int main(int Argc, const CHAR* Argv[])
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    TestHarness::Initialize();
    LOG_INFO("=== Container Tests ===");

#if RUN_TESTS
    Tests(Argc, Argv);
#endif

#if RUN_BENCHMARK
    BenchMarks();
#endif

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();

    // NOTE: If this reports no memory-leaks, that is probably false positives
    if (GMalloc)
    {
        FDebuggerOutputDevice OutputDevice;
        GMalloc->DumpAllocations(&OutputDevice);
    }

    FPlatformStackTrace::ReleaseSymbols();
    return ExitCode;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING