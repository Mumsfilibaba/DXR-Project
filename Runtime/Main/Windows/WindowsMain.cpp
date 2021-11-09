#include "Main/EngineMain.inl"

#if PLATFORM_WINDOWS

#include "Core/Debug/Debug.h"
#include "Core/Windows/Windows.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

static void InitCRunTime()
{
#ifdef DEBUG_BUILD
    uint32 DebugFlags = 0;
    DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;

    _CrtSetDbgFlag( DebugFlags );
#endif
}

int WINAPI WinMain( HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow )
{
    InitCRunTime();

    // TODO: Investigate if this is the proper way
    return EngineMain( 1, CmdLine );
}

#pragma warning(pop)

#endif