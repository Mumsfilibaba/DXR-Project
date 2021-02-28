#pragma once
#include "Time/Clock.h"

#include <Containers/ArrayView.h>

Int32 EngineMain(const TArrayView<const Char*> Args);

class EngineLoop
{
public:
    static Bool PreInit();
    static Bool Init();
    static Bool PostInit();
    
    static void PreTick();
    static void Tick();
    static void PostTick();

    static Bool PreRelease();
    static Bool Release();
    static Bool PostRelease();

    static void Exit();
    
    static Bool IsRunning();
    static Bool IsExiting();

    static Timestamp GetDeltaTime();
    static Timestamp GetTotalElapsedTime();
};