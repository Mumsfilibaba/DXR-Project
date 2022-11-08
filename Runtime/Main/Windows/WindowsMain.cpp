#include "Core/Misc/Debug.h"
#include "Core/Windows/Windows.h"

extern int32 GenericMain();

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

    return GenericMain();
}

#pragma warning(pop)