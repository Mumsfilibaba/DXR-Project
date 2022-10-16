#pragma once

struct IEngineLoop
{
    virtual ~IEngineLoop() = default;

     /** @brief - Loads all the core modules */
    virtual bool LoadCoreModules() = 0;

     /** @brief - Creates the application and load modules */
    virtual bool PreInit() = 0;

     /** @brief - Initializes and starts up the engine */
    virtual bool Init() = 0;

     /** @brief - Ticks the engine */
    virtual void Tick() = 0;

     /** @brief - Releases the engine */
    virtual bool Release() = 0;
};