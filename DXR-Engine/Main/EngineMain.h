#pragma once
#include "Core.h"

#include "Core/Engine/EngineLoop.h"
#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"

struct SEngineMainGuard
{
	~SEngineMainGuard()
	{
		if ( !EngineLoop::Release() )
		{
			PlatformApplicationMisc::MessageBox( "ERROR", "EngineLoop::Release Failed" );
		}
	}
};

// Main function for all implementations
int32 EngineMain()
{
	// Make sure that the engine is released if the main function exits early
	SEngineMainGuard EngineMainGuard;
	
    if ( !EngineLoop::Init() )
    {
		PlatformApplicationMisc::MessageBox( "ERROR", "EngineLoop::Init Failed" );
        return -1;
    }

    EngineLoop::Run();

    return 0;
}
