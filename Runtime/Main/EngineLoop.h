#pragma once
#include "Core/Time/Timer.h"

class FEngineLoop
{
public:

     /** @brief: Loads all the core modules */
    static bool LoadCoreModules();

     /** @brief: Creates the application and load modules */
    static bool PreInitialize();

     /** @brief: Initializes and starts up the engine */
    static bool Initialize();

     /** @brief: Ticks the engine */
    static void Tick(FTimespan Deltatime);

     /** @brief: Releases the engine */
    static bool Release();
};
