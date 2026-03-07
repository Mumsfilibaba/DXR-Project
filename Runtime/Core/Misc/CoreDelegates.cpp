#include "Core/Misc/CoreDelegates.h"

void CoreDelegates::Shutdown()
{
    PostInitRHIDelegate.UnbindAll();
    PostApplicationCreateDelegate.UnbindAll();
    PreInitFinishedDelegate.UnbindAll();
    PreEngineInitDelegate.UnbindAll();
    PostEngineInitDelegate.UnbindAll();
    PreApplicationLoadedDelegate.UnbindAll();
    PostGameModuleLoadedDelegate.UnbindAll();
    DeviceRemovedDelegate.UnbindAll();
}

CoreDelegates::FPostInitRHIDelegate           CoreDelegates::PostInitRHIDelegate;
CoreDelegates::FPostApplicationCreateDelegate CoreDelegates::PostApplicationCreateDelegate;
CoreDelegates::FPreInitFinishedDelegate       CoreDelegates::PreInitFinishedDelegate;
CoreDelegates::FPreEngineInitDelegate         CoreDelegates::PreEngineInitDelegate;
CoreDelegates::FPostEngineInitDelegate        CoreDelegates::PostEngineInitDelegate;
CoreDelegates::FPreApplicationLoadedDelegate  CoreDelegates::PreApplicationLoadedDelegate;
CoreDelegates::FPostApplicationLoadedDelegate CoreDelegates::PostGameModuleLoadedDelegate;
CoreDelegates::FDeviceRemovedDelegate         CoreDelegates::DeviceRemovedDelegate;
