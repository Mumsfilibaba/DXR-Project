#pragma once
#include "Core/Time/Timer.h"

class EngineLoop
{
public:
    static bool Init();

    static void Tick( CTimestamp Deltatime );

    static void Run();

    static bool Release();
};
