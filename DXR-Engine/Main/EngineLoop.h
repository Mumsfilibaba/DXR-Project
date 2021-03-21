#pragma once
#include "Time/Timer.h"

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
    
    Timer Clock;
};

extern EngineLoop GEngineLoop;