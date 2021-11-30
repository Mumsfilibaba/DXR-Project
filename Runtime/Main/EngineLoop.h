#pragma once
#include "Core/Time/Timer.h"

class CEngineLoop
{
public:

    /* Creates the application and load modules */
    static bool PreInitialize();

    /* Initializes and starts up the engine */
    static bool Initialize();

    /* Ticks the engine */
    static void Tick( CTimestamp Deltatime );

    /* Releases the engine */
    static bool Release();
};
