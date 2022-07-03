#include "EngineLoopDelegates.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EngineLoop delegates

namespace NEngineLoopDelegates
{
    CORE_API FPostInitRHIDelegate PostInitRHIDelegate;

    CORE_API FPreInitFinishedDelegate PreInitFinishedDelegate;

    CORE_API FPreEngineInitDelegate PreEngineInitDelegate;

    CORE_API FPostEngineInitDelegate PostEngineInitDelegate;

    CORE_API FPreApplicationLoadedDelegate PreApplicationLoadedDelegate;

    CORE_API FPostApplicationLoadedDelegate PostApplicationLoadedDelegate;
}
