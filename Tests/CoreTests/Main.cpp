#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/CoreGlobals.h>
#include <Core/Memory/Malloc.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/Threading/ThreadManager.h>
#include <Core/Tasks/TaskGraph.h>

#include "TaskGraphTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

int main(int Argc, const CHAR* Argv[])
{
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

    FThreadManager::Initialize();

    if (!FTaskGraph::Initialize())
    {
        return -1;
    }

    bool bResult = TaskGraph_Test();

    FTaskGraph::Release();
    FThreadManager::Release();

    return bResult ? 0 : -1;
}
