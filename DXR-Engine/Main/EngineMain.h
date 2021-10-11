#pragma once
#include "Core.h"

#include "Core/Engine/Engine.h"
#include "Core/Engine/EngineLoop.h"
#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"

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

// Main function for all implementations
int32 EngineMain()
{
    // Make sure that the engine is released if the main function exits early
    SEngineMainGuard EngineMainGuard;
    
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
    while ( CApplication::Get().IsRunning() )
    {
        Timer.Tick();
        CEngineLoop::Tick( Timer.GetDeltaTime() );
    }

    return 0;
}
