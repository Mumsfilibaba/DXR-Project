#include "EngineLoopDelegates.h"

namespace NEngineLoopDelegates
{
    CORE_API CPostInitRHIDelegate NEngineLoopDelegates::PostInitRHIDelegate;

    CORE_API CPreInitFinishedDelegate NEngineLoopDelegates::PreInitFinishedDelegate;

    CORE_API CPreEngineInitDelegate NEngineLoopDelegates::PreEngineInitDelegate;

    CORE_API CPostEngineInitDelegate NEngineLoopDelegates::PostEngineInitDelegate;

    CORE_API CPreApplicationLoadedDelegate NEngineLoopDelegates::PreApplicationLoadedDelegate;

    CORE_API CPostApplicationLoadedDelegate NEngineLoopDelegates::PostApplicationLoadedDelegate;
}