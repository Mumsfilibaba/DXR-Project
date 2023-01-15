#pragma once
#include "Core.h"

struct IOutputDevice;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Engine State

extern CORE_API bool GIsEngineExitRequested;

FORCEINLINE bool IsEngineExitRequested()
{
    return GIsEngineExitRequested;
}

extern "C" CORE_API void RequestEngineExit(const CHAR* ExitReason);

///////////////////////////////////////////////////////////////////////////////////////////////////

extern CORE_API IOutputDevice* GDebugOutput;