#include "ScopedLock.h"
#include "TaskManager.h"
#include "ThreadManager.h"

#include "Core/Platform/PlatformThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTaskManagerInterface

FTaskManagerInterface& FTaskManagerInterface::Get()
{
    static FTaskManager GInstance;
    return GInstance;
}
