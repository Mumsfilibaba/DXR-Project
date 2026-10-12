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
    TestHarness::Initialize(TEXT("TestResults_ShaderCompiler.log"));
    LOG_INFO("=== ShaderCompiler Tests ===");

    RUN_TEST(TEXT("PreprocessorMacros"), ShaderPreprocessorMacros_Test());
    RUN_TEST(TEXT("PreprocessorConditionals"), ShaderPreprocessorConditionals_Test());
    RUN_TEST(TEXT("PreprocessorIncludes"), ShaderPreprocessorIncludes_Test());
    RUN_TEST(TEXT("PreprocessorErrors"), ShaderPreprocessorErrors_Test());
    RUN_TEST(TEXT("PreprocessorGenerateSource"), ShaderPreprocessorGenerateSource_Test());
    RUN_TEST(TEXT("FXCShaderTranslator"), FXCShaderTranslator_Test());
    RUN_TEST(TEXT("ShaderSourceHashLineEndings"), ShaderSourceHashLineEndings_Test());
    RUN_TEST(TEXT("ShaderCompileJobJsonRoundTrip"), ShaderCompileJobJsonRoundTrip_Test());
    RUN_TEST(TEXT("ShaderJobFileMerge"), ShaderJobFileMerge_Test());
    RUN_TEST(TEXT("RemoteProtocolFraming"), RemoteProtocolFraming_Test());
    RUN_TEST(TEXT("RemoteProtocolSourcePaths"), RemoteProtocolSourcePaths_Test());

    if (!FThreadManager::Initialize() || !FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        TestHarness::AddFail();
    }
    else if (FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        RUN_TEST(TEXT("OutputLanguages"), ShaderCompilerOutputLanguages_Test());
        RUN_TEST(TEXT("CompileFromFile"), ShaderCompilerCompileFromFile_Test());
        RUN_TEST(TEXT("CompileFromSource"), ShaderCompilerCompileFromSource_Test());
        RUN_TEST(TEXT("CompileFailure"), ShaderCompilerCompileFailure_Test());
        RUN_TEST(TEXT("DXBCConstantsSlot"), ShaderCompilerDXBCConstantsSlot_Test());
        RUN_TEST(TEXT("CompileHash"), ShaderCompilerCompileHash_Test());
        RUN_TEST(TEXT("ReleaseOutput"), ShaderCompilerReleaseOutput_Test());
        RUN_TEST(TEXT("VertexInputs"), ShaderCompilerVertexInputs_Test());
        RUN_TEST(TEXT("RayTracing"), ShaderCompilerRayTracing_Test());
        RUN_TEST(TEXT("MSL"), ShaderCompilerMSL_Test());
        RUN_TEST(TEXT("PreprocessorMemoryProvider"), ShaderPreprocessorMemoryProvider_Test());
        RUN_TEST(TEXT("CompileJobParity"), ShaderCompilerCompileJobParity_Test());
        RUN_TEST(TEXT("ReadEmbedded"), ShaderCodeReadEmbedded_Test());
        RUN_TEST(TEXT("InternalClearBufferUAVReflection"), InternalClearBufferUAVReflection_Test());
        RUN_TEST(TEXT("InternalClearUAVMSLReflection"), InternalClearUAVMSLReflection_Test());
        RUN_TEST(TEXT("ShaderCompilerIncludeDirs"), ShaderCompilerIncludeDirs_Test());
        RUN_TEST(TEXT("RemoteShaderCompilerLoopback"), RemoteShaderCompilerLoopback_Test());
        RUN_TEST(TEXT("RemoteShaderCompilerSendsFilesOnce"), RemoteShaderCompilerSendsFilesOnce_Test());
        RUN_TEST(TEXT("HashIgnoresAssetPath"), ShaderCompilerHashIgnoresAssetPath_Test());

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

IMPLEMENT_PROGRAM_MAIN(TEXT("ShaderCompiler-Tests"), RunShaderCompilerTests);
