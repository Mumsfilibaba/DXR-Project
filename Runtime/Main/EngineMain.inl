#pragma once
#include "Core.h"

#include "Engine/Engine.h"

#include "Main/EngineLoop.h"

#include "Interface/InterfaceApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

// Main function for all implementations
int32 EngineMain()
{
    struct SEngineMainGuard
    {
        ~SEngineMainGuard()
        {
            if ( !CEngineLoop::Release() )
            {
                PlatformApplicationMisc::MessageBox( "ERROR", "CEngineLoop::Release Failed" );
            }
        }
    };

    // Make sure that the engine is released if the main function exits early
    SEngineMainGuard EngineMainGuard;
    
    // Run loop
    if ( !CEngineLoop::PreInit() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "CEngineLoop::PreInit Failed" );
        return -1;
    }

    if ( !CEngineLoop::Init() )
    {
        PlatformApplicationMisc::MessageBox( "ERROR", "CEngineLoop::Init Failed" );
        return -1;
    }

    CTimer Timer;
    while ( CInterfaceApplication::Get().IsRunning() )
    {
        Timer.Tick();
        CEngineLoop::Tick( Timer.GetDeltaTime() );
    }

    return 0;
}
