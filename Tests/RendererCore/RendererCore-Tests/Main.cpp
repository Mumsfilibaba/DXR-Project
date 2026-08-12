#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Threading/ThreadManager.h>
#include <RHI/RHI.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include "RenderGraphTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

static void SetConsoleVariable(const CHAR* VariableName, bool bValue)
{
    if (IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable(VariableName))
    {
        Variable->SetAsBool(bValue, EConsoleVariableFlags::SetByCode);
    }
    else
    {
        LOG_WARNING("Console variable '%s' was not found", VariableName);
    }
}

static void SetConsoleVariable(const CHAR* VariableName, const CHAR* Value)
{
    if (IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable(VariableName))
    {
        Variable->SetString(Value, EConsoleVariableFlags::SetByCode);
    }
    else
    {
        LOG_WARNING("Console variable '%s' was not found", VariableName);
    }
}

int main(int Argc, const CHAR* Argv[])
{
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

    TestHarness::Initialize("TestResults_RendererCore.log");
    LOG_INFO("=== RendererCore Tests ===");

    SetConsoleVariable("RHI.Type", "Null");
    SetConsoleVariable("RHI.EnableValidation", true);
    SetConsoleVariable("RHI.EnableResourceStateValidation", true);
    SetConsoleVariable("RHI.EnableValidationDebugBreak", false);
    SetConsoleVariable("TaskGraph.EnableRHIThread", false);

    if (!FThreadManager::Initialize())
    {
        LOG_ERROR("Failed to initialize the thread manager");
        TestHarness::Shutdown();
        return -1;
    }

    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("Failed to initialize the task graph");
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

    if (!RHI::Initialize())
    {
        LOG_ERROR("Failed to initialize the RHI");
        FTaskGraph::Release();
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

    if (!FRenderGraphResourcePool::Initialize())
    {
        LOG_ERROR("Failed to initialize the render-graph resource pool");
        RHI::Release();
        FTaskGraph::Release();
        FThreadManager::Release();
        TestHarness::Shutdown();
        return -1;
    }

    RUN_TEST("RenderGraphValidationSelfCheck", RenderGraphValidationSelfCheck_Test());
    RUN_TEST("RenderGraph", RenderGraph_Test());
    RUN_TEST("RenderGraphFrame", RenderGraphFrame_Test());

    FRenderGraphResourcePool::Release();
    RHI::Release();
    FTaskGraph::Release();
    FThreadManager::Release();

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}
