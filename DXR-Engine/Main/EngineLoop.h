#pragma once
#include "Time/Clock.h"

#include "Core/Containers/ArrayView.h"

int32 EngineMain();

class EngineLoop
{
public:
    static bool PreInit();
    static bool Init();
    static bool PostInit();
    
    static void PreTick();
    static void Tick();
    static void PostTick();

    static bool PreRelease();
    static bool Release();
    static bool PostRelease();

    static void Exit();
    
    static bool IsRunning();
    static bool IsExiting();

    static Timestamp GetDeltaTime();
    static Timestamp GetTotalElapsedTime();
};