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

// D3D12 Agility SDK exports
#if D3D12_AGILITY_SDK_EXPORTS
    extern "C"
    {
        // Version of the D3D12 Agility SDK
        __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_AGILITY_SDK_VERSION;

        // Path to the D3D12Core.dll
        __declspec(dllexport) extern const char* D3D12SDKPath = D3D12_AGILITY_SDK_PATH;
    }
#endif

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int CmdShow)
{
    InitCRunTime();

    const CHAR* LocalCommandLine = CommandLine;
    return EngineMain(&LocalCommandLine, 1);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING