#pragma once
#include "Time/Clock.h"

#include <Containers/ArrayView.h>

Int32 EngineMain(const TArrayView<const Char*> Args);

class EngineLoop
{
public:
    EngineLoop()  = default;
    ~EngineLoop() = default;

    Bool PreInit();
    Bool Init();
    Bool PostInit();
    
    void PreTick();
    void Tick();
    void PostTick();

    Bool PreRelease();
    Bool Release();
    Bool PostRelease();

    void Exit();
    
    Bool IsRunning() const;

    Timestamp GetDeltaTime()        const;
    Timestamp GetTotalElapsedTime() const;

private:
    Bool  ShouldRun = false;
    Clock Clock;
};