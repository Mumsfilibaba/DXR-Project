#include "Windows/Windows.h"

#include "Main/EngineMain.h"

#include "Core/Application/Windows/WindowsApplication.h"

#include "Debug/Debug.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

static void InitCRunTime()
{
    uint32 DebugFlags = 0;
#ifdef DEBUG_BUILD
    DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
#endif

    _CrtSetDbgFlag(DebugFlags);
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    InitCRunTime();

    WindowsApplication::PreMainInit(Instance);

    return EngineMain();
}

#pragma warning(pop)