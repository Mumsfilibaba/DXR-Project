#pragma once
#include "Core/Time/Stopwatch.h"

struct FOutputDeviceConsole;

class FEngineLoop
{
public:
    FEngineLoop();
    ~FEngineLoop();

    bool LoadCoreModules();

    bool PreInitialize();

    bool Initialize();

    void Tick();

    bool Release();

private:
    FStopwatch            FrameTimer;
    FOutputDeviceConsole* ConsoleWindow;
};

extern FEngineLoop GEngineLoop;