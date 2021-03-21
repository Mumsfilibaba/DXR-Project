#pragma once
#include "Time/Clock.h"

class EngineLoop
{
public:
    bool Init();
    void Tick();
    bool Release();

    Timestamp GetDeltaTime();
    Timestamp GetRunningTime();

private:
    bool PreInit();
    bool PostInit();
    
    bool PreRelease();
    bool PostRelease();
    
    Clock Clock;
};

extern EngineLoop GEngineLoop;