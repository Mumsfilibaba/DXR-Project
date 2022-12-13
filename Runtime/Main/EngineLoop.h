#pragma once
#include "IEngineLoop.h"

#include "Core/Time/Stopwatch.h"

struct FOutputDeviceConsole;

class FEngineLoop 
    : public IEngineLoop
{
public:
    FEngineLoop();
    ~FEngineLoop();

     /** @brief - Loads all the core modules */
    virtual bool LoadCoreModules() override final;

     /** @brief - Creates the application and load modules */
    virtual bool PreInit() override final;

     /** @brief - Initializes and starts up the engine */
    virtual bool Init() override final;

     /** @brief - Ticks the engine */
    virtual void Tick() override final;

     /** @brief - Releases the engine */
    virtual bool Release() override final;

private:
    FStopwatch            FrameTimer;
    FOutputDeviceConsole* ConsoleWindow;
};
