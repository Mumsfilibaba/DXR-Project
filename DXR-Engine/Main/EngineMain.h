#pragma once
#include "Core.h"

#include "Core/Engine/EngineLoop.h"
#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformMisc.h"

int32 EngineMain()
{
    if ( !EngineLoop::Init() )
    {
        PlatformMisc::MessageBox( "ERROR", "EngineLoop::Init Failed" );
        return -1;
    }

    EngineLoop::Run();

    if ( !EngineLoop::Release() )
    {
        PlatformMisc::MessageBox( "ERROR", "EngineLoop::Release Failed" );
        return -1;
    }

    return 0;
}