#include "CoreDelegates.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EngineLoop delegates

namespace NCoreDelegates
{
    CORE_API FPostInitRHIDelegate PostInitRHIDelegate;

    CORE_API FPreInitFinishedDelegate PreInitFinishedDelegate;

    CORE_API FPreEngineInitDelegate PreEngineInitDelegate;

    CORE_API FPostEngineInitDelegate PostEngineInitDelegate;

    CORE_API FPreApplicationLoadedDelegate PreApplicationLoadedDelegate;

    CORE_API FPostApplicationLoadedDelegate PostApplicationLoadedDelegate;

    CORE_API FDeviceRemovedDelegate DeviceRemovedDelegate;
}
