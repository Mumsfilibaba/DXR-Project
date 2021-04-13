#include "Windows/Windows.h"

#include "Main/EngineMain.h"

#include "Core/Application/Windows/WindowsPlatform.h"

#include "Debug/Debug.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

static void InitCRunTime()
{
#ifdef DEBUG_BUILD
    uint32 DebugFlags = 0;
    DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;

    _CrtSetDbgFlag(DebugFlags);
#endif
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    InitCRunTime();

    WindowsPlatform::PreMainInit(Instance);

    return EngineMain();
}

#pragma warning(pop)