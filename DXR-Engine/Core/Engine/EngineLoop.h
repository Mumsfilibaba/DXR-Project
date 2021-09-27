#pragma once
#include "Core/Time/Timer.h"

class CEngineLoop
{
public:

    CEngineLoop();
    virtual ~CEngineLoop();

    /* Creates the application and load modules */
    virtual bool PreInit();

    /* Initializes and starts up the engine */
    virtual bool Init();

    /* Ticks the engine */
    virtual void Tick( CTimestamp Deltatime );

    /* Releases the engine */
    virtual bool Release();
};

extern CEngineLoop GEngineLoop;