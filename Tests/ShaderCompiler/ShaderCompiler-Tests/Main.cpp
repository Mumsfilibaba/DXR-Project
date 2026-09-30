#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Misc/Paths.h>
#include <ShaderCompiler/ShaderCompiler.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "ShaderCompilerTests.h"
#include "ShaderPreprocessorTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static int32 RunShaderCompilerTests()
{
    TestHarness::Initialize("TestResults_ShaderCompiler.log");
    LOG_INFO("=== ShaderCompiler Tests ===");

    RUN_TEST("PreprocessorMacros", ShaderPreprocessorMacros_Test());
    RUN_TEST("PreprocessorConditionals", ShaderPreprocessorConditionals_Test());
    RUN_TEST("PreprocessorIncludes", ShaderPreprocessorIncludes_Test());
    RUN_TEST("PreprocessorErrors", ShaderPreprocessorErrors_Test());
    RUN_TEST("PreprocessorRender", ShaderPreprocessorRender_Test());
    RUN_TEST("FXCShaderTranslator", FXCShaderTranslator_Test());

    if (FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        RUN_TEST("OutputLanguages", ShaderCompilerOutputLanguages_Test());
        RUN_TEST("CompileFromFile", ShaderCompilerCompileFromFile_Test());
        RUN_TEST("CompileFromSource", ShaderCompilerCompileFromSource_Test());
        RUN_TEST("CompileFailure", ShaderCompilerCompileFailure_Test());
        RUN_TEST("DXBCConstantsSlot", ShaderCompilerDXBCConstantsSlot_Test());
        RUN_TEST("CompileHash", ShaderCompilerCompileHash_Test());

        FShaderCompiler::Destroy();
    }
    else
    {
        LOG_ERROR("Failed to initialize the ShaderCompiler");
        TestHarness::AddFail();
    }

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN("ShaderCompiler-Tests", RunShaderCompilerTests);
