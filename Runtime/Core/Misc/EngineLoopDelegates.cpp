#include "EngineLoopDelegates.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EngineLoop delegates

namespace NEngineLoopDelegates
{
    CORE_API CPostInitRHIDelegate PostInitRHIDelegate;

    CORE_API CPreInitFinishedDelegate PreInitFinishedDelegate;

    CORE_API CPreEngineInitDelegate PreEngineInitDelegate;

    CORE_API CPostEngineInitDelegate PostEngineInitDelegate;

    CORE_API CPreApplicationLoadedDelegate PreApplicationLoadedDelegate;

    CORE_API CPostApplicationLoadedDelegate PostApplicationLoadedDelegate;
}
