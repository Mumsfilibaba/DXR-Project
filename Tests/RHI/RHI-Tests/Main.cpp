#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include "RHIValidationHelperTests.h"
#include "ShaderPermutationTests.h"
#include "VertexDeclarationTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

int main(int Argc, const CHAR* Argv[])
{
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

    TestHarness::Initialize("TestResults_RHI.log");
    LOG_INFO("=== RHI Tests ===");

    RUN_TEST("RHIValidationHelpers", RHIValidationHelpers_Test());
    RUN_TEST("ShaderPermutation", ShaderPermutation_Test());
    RUN_TEST("VertexDeclaration", VertexDeclaration_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}
