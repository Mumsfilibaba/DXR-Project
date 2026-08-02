#pragma once
#include "Core/Core.h"

struct FMalloc;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Engine State

/** True once shutdown has been requested: the engine loop finishes its current frame and exits */
extern CORE_API bool GIsEngineExitRequested;

FORCEINLINE bool IsEngineExitRequested()
{
    return GIsEngineExitRequested;
}

extern "C" CORE_API void RequestEngineExit(const CHAR* ExitReason);

/** True when no user is present to answer a dialog: automated tests, CI, -unattended */
extern CORE_API bool GIsUnattended;

FORCEINLINE bool IsUnattended()
{
    return GIsUnattended;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

extern CORE_API FMalloc* GMalloc;
