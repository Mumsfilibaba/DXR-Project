#pragma once
#include "Main/EngineLoop.h"

#include "Core/Core.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Canvas/CanvasApplication.h"

#include "Engine/Engine.h"

// Main function for all implementations
int32 EngineMain()
{
    struct SEngineMainGuard
    {
        ~SEngineMainGuard()
        {
            if (!CEngineLoop::Release())
            {
                PlatformApplicationMisc::MessageBox("ERROR", "CEngineLoop::Release Failed");
            }
        }
    };

    // Make sure that the engine is released if the main function exits early
    SEngineMainGuard EngineMainGuard;

    // Initialization
    if (!CEngineLoop::PreInitialize())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "CEngineLoop::PreInit Failed");
        return -1;
    }

    if (!CEngineLoop::Initialize())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "CEngineLoop::Init Failed");
        return -1;
    }

    // Run loop
    CTimer Timer;
    while (CCanvasApplication::Get().IsRunning())
    {
        Timer.Tick();
        CEngineLoop::Tick(Timer.GetDeltaTime());
    }

    return 0;
}
