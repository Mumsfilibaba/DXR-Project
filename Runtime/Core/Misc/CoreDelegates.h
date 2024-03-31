#pragma once
#include "Core/Core.h"
#include "Core/Delegates/MulticastDelegate.h"

namespace CoreDelegates
{
    /**
     * @brief - Delegate that gets called after the RHI is initialized 
     */
    DECLARE_MULTICAST_DELEGATE(FPostInitRHIDelegate);
    extern CORE_API FPostInitRHIDelegate PostInitRHIDelegate;

    /**
     * @brief - Delegate that gets called after the Application is created
     */
    DECLARE_MULTICAST_DELEGATE(FPostApplicationCreateDelegate);
    extern CORE_API FPostApplicationCreateDelegate PostApplicationCreateDelegate;

    /**
     * @brief - Delegate that gets called at the end of PreInit
     */
    DECLARE_MULTICAST_DELEGATE(FPreInitFinishedDelegate);
    extern CORE_API FPreInitFinishedDelegate PreInitFinishedDelegate;

    /**
     * @brief - Delegate that gets called before the Engine is created 
     */
    DECLARE_MULTICAST_DELEGATE(FPreEngineInitDelegate);
    extern CORE_API FPreEngineInitDelegate PreEngineInitDelegate;

    /**
     * @brief - Delegate that gets called after the Engine is created 
     */
    DECLARE_MULTICAST_DELEGATE(FPostEngineInitDelegate);
    extern CORE_API FPostEngineInitDelegate PostEngineInitDelegate;

    /**
     * @brief - Delegate that gets called before the Application-Module is loaded 
     */
    DECLARE_MULTICAST_DELEGATE(FPreApplicationLoadedDelegate);
    extern CORE_API FPreApplicationLoadedDelegate PreApplicationLoadedDelegate;

    /**
     * @brief - Delegate that gets called after the Application-Module is loaded
     */
    DECLARE_MULTICAST_DELEGATE(FPostApplicationLoadedDelegate);
    extern CORE_API FPostApplicationLoadedDelegate PostGameModuleLoadedDelegate;

    /**
     * @brief - Delegate that gets called after the Application-Module is loaded
     */
    DECLARE_MULTICAST_DELEGATE(FDeviceRemovedDelegate);
    extern CORE_API FDeviceRemovedDelegate DeviceRemovedDelegate;
};