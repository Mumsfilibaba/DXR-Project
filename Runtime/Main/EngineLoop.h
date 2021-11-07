#pragma once
#include "Core/Time/Timer.h"

class CEngineLoop
{
public:

	// Initialize the commandlin by parsing all the arguments 
	static void InitCommandLineArgs(  int32 NumCommandLineArgs, const char** CommandLineArgs );
	
    /* Creates the application and load modules */
    static bool PreInit();

    /* Initializes and starts up the engine */
    static bool Init();

    /* Ticks the engine */
    static void Tick( CTimestamp Deltatime );

    /* Releases the engine */
    static bool Release();
};
