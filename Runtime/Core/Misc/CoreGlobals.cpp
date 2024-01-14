#include "OutputDeviceLogger.h"
#include "Core/CoreGlobals.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Engine State

bool GIsEngineExitRequested = false;

void RequestEngineExit(const CHAR* ExitReason)
{
    LOG_INFO("Engine Exit is requested. Reason '%s'", ExitReason);
    GIsEngineExitRequested = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CORE_API FMalloc* GMalloc = nullptr;

CORE_API IOutputDevice* GDebugOutput = nullptr;

CORE_API FGameModule* GGameModule  = nullptr;
