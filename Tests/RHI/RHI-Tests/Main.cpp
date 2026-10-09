#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "HDRMetadataTests.h"
#include "MSLShaderBindingTests.h"
#include "PrimitiveTopologyTests.h"
#include "RHIValidationHelperTests.h"
#include "ShaderPermutationTests.h"
#include "VertexDeclarationTests.h"

static int32 RunRHITests()
{
    TestHarness::Initialize("TestResults_RHI.log");
    LOG_INFO("=== RHI Tests ===");

    RUN_TEST("RHIValidationHelpers", RHIValidationHelpers_Test());
    RUN_TEST("ShaderPermutation", ShaderPermutation_Test());
    RUN_TEST("VertexDeclaration", VertexDeclaration_Test());
    RUN_TEST("PrimitiveTopology", PrimitiveTopology_Test());
    RUN_TEST("HDRMetadata", HDRMetadata_Test());
    RUN_TEST("MSLShaderBinding", MSLShaderBinding_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN("RHI-Tests", RunRHITests);
