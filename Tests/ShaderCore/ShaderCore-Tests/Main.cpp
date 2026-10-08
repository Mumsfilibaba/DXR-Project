#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "ShaderCodeTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static int32 RunShaderCoreTests()
{
    TestHarness::Initialize("TestResults_ShaderCore.log");
    LOG_INFO("=== ShaderCore Tests ===");

    RUN_TEST("ShaderCodeRoundTrip", ShaderCodeRoundTrip_Test());
    RUN_TEST("ShaderCodeLanguageBlocks", ShaderCodeLanguageBlocks_Test());
    RUN_TEST("ShaderCodeDebugInfo", ShaderCodeDebugInfo_Test());
    RUN_TEST("ShaderCodeVertexInputs", ShaderCodeVertexInputs_Test());
    RUN_TEST("ShaderCodeReaderRejects", ShaderCodeReaderRejects_Test());
    RUN_TEST("ShaderCodeWriterRejects", ShaderCodeWriterRejects_Test());
    RUN_TEST("ShaderSemanticHash", ShaderSemanticHash_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN("ShaderCore-Tests", RunShaderCoreTests);
