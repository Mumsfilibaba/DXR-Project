#include "Launch/EngineLoop.h"
#include "Core/Core.h"
#include "Core/CoreGlobals.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Memory/Malloc.h"
#include "Core/Platform/PlatformMisc.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

// EngineLoop
FEngineLoop GEngineLoop;

FORCEINLINE int32 EnginePreInit(const CHAR** Args, int32 NumArgs)
{
    int32 ErrorCode = GEngineLoop.PreInit(Args, NumArgs);
    return ErrorCode;
}

FORCEINLINE int32 EngineInit()
{
    int32 ErrorCode = GEngineLoop.Init();
    return ErrorCode;
}

FORCEINLINE void EngineTick()
{
    GEngineLoop.Tick();
}

FORCEINLINE void EngineRelease()
{
    GEngineLoop.Release();
}

int32 EngineMain(const CHAR* Args[], int32 NumArgs)
{
    // Make sure that the engine is released if the main function exits early
    struct FEngineReleaseGuard
    {
        ~FEngineReleaseGuard()
        {
            EngineRelease();
        }
    } EngineReleaseGuard;

    int32 ErrorCode = EnginePreInit(Args, NumArgs);
    if (ErrorCode != 0)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::PreInit Failed");
        return ErrorCode;
    }

    ErrorCode = EngineInit();
    if (ErrorCode != 0)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::Init Failed");
        return ErrorCode;
    }

    // Run loop
    while (!IsEngineExitRequested())
    {
        EngineTick();
    }

    return ErrorCode;
}
