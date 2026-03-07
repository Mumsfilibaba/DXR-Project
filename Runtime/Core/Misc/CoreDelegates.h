#pragma once
#include "Core/Core.h"
#include "Core/Delegates/MulticastDelegate.h"

struct CORE_API CoreDelegates
{
    /**
     * @brief Unbinds all core delegates. Must be called before module DLLs are unloaded
     *        to avoid dangling vtable pointers in delegate instances.
     */
    static void Shutdown();

    /**
     * @brief Delegate that gets called after the RHI is initialized 
     */
    DECLARE_MULTICAST_DELEGATE(FPostInitRHIDelegate);
    static FPostInitRHIDelegate PostInitRHIDelegate;

    /**
     * @brief Delegate that gets called after the Application is created
     */
    DECLARE_MULTICAST_DELEGATE(FPostApplicationCreateDelegate);
    static FPostApplicationCreateDelegate PostApplicationCreateDelegate;

    /**
     * @brief Delegate that gets called at the end of PreInit
     */
    DECLARE_MULTICAST_DELEGATE(FPreInitFinishedDelegate);
    static FPreInitFinishedDelegate PreInitFinishedDelegate;

    /**
     * @brief Delegate that gets called before the Engine is created 
     */
    DECLARE_MULTICAST_DELEGATE(FPreEngineInitDelegate);
    static FPreEngineInitDelegate PreEngineInitDelegate;

    /**
     * @brief Delegate that gets called after the Engine is created 
     */
    DECLARE_MULTICAST_DELEGATE(FPostEngineInitDelegate);
    static FPostEngineInitDelegate PostEngineInitDelegate;

    /**
     * @brief Delegate that gets called before the Application-Module is loaded 
     */
    DECLARE_MULTICAST_DELEGATE(FPreApplicationLoadedDelegate);
    static FPreApplicationLoadedDelegate PreApplicationLoadedDelegate;

    /**
     * @brief Delegate that gets called after the Application-Module is loaded
     */
    DECLARE_MULTICAST_DELEGATE(FPostApplicationLoadedDelegate);
    static FPostApplicationLoadedDelegate PostGameModuleLoadedDelegate;

    /**
     * @brief Delegate that gets called after the Application-Module is loaded
     */
    DECLARE_MULTICAST_DELEGATE(FDeviceRemovedDelegate);
    static FDeviceRemovedDelegate DeviceRemovedDelegate;
};
