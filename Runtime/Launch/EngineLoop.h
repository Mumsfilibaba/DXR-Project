#pragma once
#include "Core/Time/Stopwatch.h"

class FEngineLoop
{
public:
    FEngineLoop();
    ~FEngineLoop();

    /* Initialize engine systems needed by other systems */
    int32 PreInit(const CHAR** Args, int32 NumArgs);

    /* Initialize engine systems */
    int32 Init();

    /* Load up modules always needed */
    bool LoadCoreModules();

    /* Advance the engine a frame */
    void Tick();

    /* Release engine systems */
    void Release();

private:
    FStopwatch FrameTimer;
};

extern FEngineLoop GEngineLoop;