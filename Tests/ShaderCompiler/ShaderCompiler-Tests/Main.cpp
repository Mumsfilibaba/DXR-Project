#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Misc/Paths.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Threading/ThreadManager.h>
#include <ShaderCompiler/ShaderCompiler.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

#include "RemoteShaderCompilerTests.h"
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
    RUN_TEST("ShaderSourceHashLineEndings", ShaderSourceHashLineEndings_Test());
    RUN_TEST("ShaderCompileJobJsonRoundTrip", ShaderCompileJobJsonRoundTrip_Test());
    RUN_TEST("ShaderJobFileMerge", ShaderJobFileMerge_Test());
    RUN_TEST("RemoteProtocolFraming", RemoteProtocolFraming_Test());
    RUN_TEST("RemoteProtocolSourcePaths", RemoteProtocolSourcePaths_Test());

    if (!FThreadManager::Initialize() || !FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        TestHarness::AddFail();
    }
    else if (FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        RUN_TEST("OutputLanguages", ShaderCompilerOutputLanguages_Test());
        RUN_TEST("CompileFromFile", ShaderCompilerCompileFromFile_Test());
        RUN_TEST("CompileFromSource", ShaderCompilerCompileFromSource_Test());
        RUN_TEST("CompileFailure", ShaderCompilerCompileFailure_Test());
        RUN_TEST("DXBCConstantsSlot", ShaderCompilerDXBCConstantsSlot_Test());
        RUN_TEST("CompileHash", ShaderCompilerCompileHash_Test());
        RUN_TEST("ReleaseOutput", ShaderCompilerReleaseOutput_Test());
        RUN_TEST("VertexInputs", ShaderCompilerVertexInputs_Test());
        RUN_TEST("RayTracing", ShaderCompilerRayTracing_Test());
        RUN_TEST("MSL", ShaderCompilerMSL_Test());
        RUN_TEST("PreprocessorMemoryProvider", ShaderPreprocessorMemoryProvider_Test());
        RUN_TEST("CompileJobParity", ShaderCompilerCompileJobParity_Test());
        RUN_TEST("ReadEmbedded", ShaderCodeReadEmbedded_Test());
        RUN_TEST("InternalClearBufferUAVReflection", InternalClearBufferUAVReflection_Test());
        RUN_TEST("InternalClearUAVMSLReflection", InternalClearUAVMSLReflection_Test());
        RUN_TEST("ShaderCompilerIncludeDirs", ShaderCompilerIncludeDirs_Test());
        RUN_TEST("RemoteShaderCompilerLoopback", RemoteShaderCompilerLoopback_Test());
        RUN_TEST("RemoteShaderCompilerSendsFilesOnce", RemoteShaderCompilerSendsFilesOnce_Test());
        RUN_TEST("HashIgnoresAssetPath", ShaderCompilerHashIgnoresAssetPath_Test());

        FShaderCompiler::Destroy();
    }
    else
    {
        LOG_ERROR("Failed to initialize the ShaderCompiler");
        TestHarness::AddFail();
    }

    FTaskGraph::Release();
    FThreadManager::Release();

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN("ShaderCompiler-Tests", RunShaderCompilerTests);
