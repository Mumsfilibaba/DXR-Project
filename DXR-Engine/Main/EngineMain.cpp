#include "EngineMain.h"
#include "EngineLoop.h"

#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformMisc.h"

int32 EngineMain()
{
    if (!GEngineLoop.Init())
    {
        PlatformMisc::MessageBox("ERROR", "EngineLoop::Init Failed");
        return -1;
    }

    while (GApplication->IsRunning)
    {
        GEngineLoop.Tick();
    }

    if (!GEngineLoop.Release())
    {
        PlatformMisc::MessageBox("ERROR", "EngineLoop::Release Failed");
        return -1;
    }

    return 0;
}
