#include "Main/EngineLoop.h"

#include "Core/Core.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Canvas/Application.h"

#include "Engine/Engine.h"

// Main function for all implementations
int32 EngineMain()
{
    struct SEngineMainGuard
    {
        ~SEngineMainGuard()
        {
            if (!FEngineLoop::Release())
            {
                FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::Release Failed");
            }
        }
    };

    // Make sure that the engine is released if the main function exits early
    SEngineMainGuard EngineMainGuard;

    // Initialization
    if (!FEngineLoop::PreInitialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::PreInit Failed");
        return -1;
    }

    if (!FEngineLoop::Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::Init Failed");
        return -1;
    }

    // Run loop
    FTimer Timer;
    while (FApplication::Get().IsRunning())
    {
        Timer.Tick();
        FEngineLoop::Tick(Timer.GetDeltaTime());
    }

    return 0;
}
