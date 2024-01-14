#include "Core/Misc/Debug.h"
#include "Core/Windows/Windows.h"

extern int32 EngineMain(const CHAR* Args[], int32 NumArgs);

DISABLE_UNREFERENCED_VARIABLE_WARNING

static void InitCRunTime()
{
#ifdef DEBUG_BUILD
    uint32 DebugFlags = 0;
    DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(DebugFlags);
#endif
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int CmdShow)
{
    InitCRunTime();

    const CHAR* TempCommandLine = CommandLine;
    return EngineMain(&TempCommandLine, 1);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING