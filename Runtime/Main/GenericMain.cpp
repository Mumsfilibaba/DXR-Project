#include "Main/EngineLoop.h"

#include "Core/Core.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Application/ApplicationInterface.h"

#include "Engine/Engine.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GEngineLoop

FEngineLoop GEngineLoop;

FORCEINLINE bool EngineLoadCoreModules()
{
    return GEngineLoop.LoadCoreModules();
}

FORCEINLINE bool EnginePreInit()
{
    return GEngineLoop.PreInit();
}

FORCEINLINE bool EngineInit()
{
    return GEngineLoop.Init();
}

FORCEINLINE void EngineTick()
{
    GEngineLoop.Tick();
}

FORCEINLINE bool EngineRelease()
{
    return GEngineLoop.Release();
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GenericMain

int32 GenericMain()
{
    struct FGenericMainGuard
    {
        ~FGenericMainGuard()
        {
            if (!EngineRelease())
            {
                FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::Release Failed");
            }
        }
    };

    {
        // Make sure that the engine is released if the main function exits early
        FGenericMainGuard GenericMainGuard;

        if (!EnginePreInit())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::PreInit Failed");
            return -1;
        }

        if (!EngineInit())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::Init Failed");
            return -1;
        }

        // Run loop
        while (FApplicationInterface::Get().IsRunning())
        {
            EngineTick();
        }
    }

    return 0;
}
