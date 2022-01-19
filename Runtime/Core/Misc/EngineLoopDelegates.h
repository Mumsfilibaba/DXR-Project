#pragma once
#include "Core/CoreModule.h"
#include "Core/Delegates/MulticastDelegate.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Delegates for different stages of the engine-loop

namespace NEngineLoopDelegates
{
    /* Delegate that gets called after the RHI is initialized */
    DECLARE_MULTICAST_DELEGATE(CPostInitRHIDelegate);
    extern CORE_API CPostInitRHIDelegate PostInitRHIDelegate;

    /* Delegate that gets called at the end of PreInit */
    DECLARE_MULTICAST_DELEGATE(CPreInitFinishedDelegate);
    extern CORE_API CPreInitFinishedDelegate PreInitFinishedDelegate;

    /* Delegate that gets called before the Engine is created */
    DECLARE_MULTICAST_DELEGATE(CPreEngineInitDelegate);
    extern CORE_API CPreEngineInitDelegate PreEngineInitDelegate;

    /* Delegate that gets called after the Engine is created */
    DECLARE_MULTICAST_DELEGATE(CPostEngineInitDelegate);
    extern CORE_API CPostEngineInitDelegate PostEngineInitDelegate;

    /* Delegate that gets called before the Application-Module is loaded */
    DECLARE_MULTICAST_DELEGATE(CPreApplicationLoadedDelegate);
    extern CORE_API CPreApplicationLoadedDelegate PreApplicationLoadedDelegate;

    /* Delegate that gets called after the Application-Module is loaded */
    DECLARE_MULTICAST_DELEGATE(CPostApplicationLoadedDelegate);
    extern CORE_API CPostApplicationLoadedDelegate PostApplicationLoadedDelegate;
};